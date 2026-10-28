/* BLE discovery flow adapted from Zephyr central_hr:
 * Copyright (c) 2015-2016 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <ant_interface.h>
#include <ant_parameters.h>
#include <ant_key_manager.h>
#include <ant_profiles/hrm/ant_hrm.h>
#include "hr_measurement.h"

LOG_MODULE_REGISTER(relay, LOG_LEVEL_INF);
#define STALE_MS 5000
static struct bt_conn *peer;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discovery;
static struct bt_gatt_subscribe_params subscription;
static struct k_spinlock sample_lock;
static struct {
    uint8_t bpm, beats;
    uint16_t beat_time;
    uint32_t fraction;
    int64_t received, advanced;
    bool valid, rr;
} sample;
static ant_hrm_profile_t hrm;
K_MSGQ_DEFINE(ant_events, sizeof(ant_evt_t), 32, 4);
static atomic_t ant_overflow;
static bool ant_open, ant_closing;
static void profile_event(ant_hrm_profile_t *profile, ant_hrm_evt_t event);
HRM_SENS_PROFILE_CONFIG_DEF(relay, true, ANT_HRM_PAGE_0, profile_event);
/* Evaluation identity: device number 4242; manufacturer 255 = development. */
HRM_SENS_CHANNEL_CONFIG_DEF(relay, 0, 1, 4242, 0);

static void timeout_disconnect(struct bt_conn *conn, void *context)
{
    ARG_UNUSED(context);
    bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}
static void receive_timeout(struct k_work *work)
{
    ARG_UNUSED(work);
    LOG_WRN("No valid heart rate notification for 15 seconds; reconnecting");
    bt_conn_foreach(BT_CONN_TYPE_LE, timeout_disconnect, NULL);
}
K_WORK_DELAYABLE_DEFINE(receive_watchdog, receive_timeout);

static void subscribed(struct bt_conn *conn, uint8_t err,
                       struct bt_gatt_subscribe_params *params)
{
    ARG_UNUSED(params);
    if (err) {
        LOG_WRN("Heart rate notification subscription rejected: %u", err);
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    } else LOG_INF("Heart rate notifications enabled");
}

static void invalidate(void)
{
    k_spinlock_key_t key = k_spin_lock(&sample_lock);
    sample.valid = false;
    k_spin_unlock(&sample_lock, key);
}
static void profile_event(ant_hrm_profile_t *profile, ant_hrm_evt_t event)
{
    ARG_UNUSED(event);
    int64_t now = k_uptime_get();
    k_spinlock_key_t key = k_spin_lock(&sample_lock);
    bool fresh = sample.valid && now - sample.received < STALE_MS;
    if (fresh && !sample.rr) {
        /* No RR data: advance an estimated beat clock, never claim real HRV. */
        uint64_t units = sample.fraction + (uint64_t)(now - sample.advanced) * sample.bpm;
        uint32_t beats = units / 60000;
        sample.fraction = units % 60000;
        sample.beats += beats;
        sample.beat_time += beats * (61440U / sample.bpm);
        sample.advanced = now;
    }
    profile->HRM_PROFILE_computed_heart_rate = fresh ? sample.bpm : 0;
    profile->HRM_PROFILE_beat_count = sample.beats;
    profile->HRM_PROFILE_beat_time = sample.beat_time;
    profile->HRM_PROFILE_operating_time = now / 2000;
    k_spin_unlock(&sample_lock, key);
}
static uint8_t notified(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                        const void *data, uint16_t length)
{
    if (!data) {
        params->value_handle = 0;
        invalidate();
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        return BT_GATT_ITER_STOP;
    }
    struct hr_measurement value;
    if (!hr_parse(data, length, &value)) {
        LOG_WRN("Malformed or unsupported heart rate measurement (%u bytes)", length);
        return BT_GATT_ITER_CONTINUE;
    }
    int64_t now = k_uptime_get();
    k_spinlock_key_t key = k_spin_lock(&sample_lock);
    bool reset = !sample.valid || now - sample.received >= STALE_MS;
    sample.bpm = value.bpm;
    sample.valid = value.bpm != 0 && !value.contact_lost;
    sample.received = now;
    sample.rr = value.rr_count != 0;
    if (reset) {
        sample.advanced = now;
        sample.fraction = 0;
        sample.beat_time = (uint16_t)(now * 1024 / 1000 - value.rr_ticks);
    }
    if (sample.valid && sample.rr) {
        sample.beats += value.rr_count;
        sample.beat_time += value.rr_ticks;
        sample.advanced = now;
        sample.fraction = 0;
    }
    k_spin_unlock(&sample_lock, key);
    k_work_reschedule(&receive_watchdog, K_SECONDS(15));
    LOG_INF("BLE HR %u bpm, RR intervals %u, contact %s", value.bpm,
            value.rr_count, value.contact_lost ? "lost" : "ok/unknown");
    return BT_GATT_ITER_CONTINUE;
}
static uint8_t discovered(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                          struct bt_gatt_discover_params *params)
{
    int err;
    if (!attr) {
        LOG_WRN("Required heart rate attribute missing");
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        return BT_GATT_ITER_STOP;
    }
    if (params->type == BT_GATT_DISCOVER_PRIMARY) {
        const struct bt_gatt_service_val *service = attr->user_data;
        discovery.end_handle = service->end_handle;
        uuid.val = BT_UUID_HRS_MEASUREMENT_VAL;
        discovery.type = BT_GATT_DISCOVER_CHARACTERISTIC;
        discovery.start_handle = attr->handle + 1;
    } else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
        const struct bt_gatt_chrc *chrc = attr->user_data;
        if (!(chrc->properties & BT_GATT_CHRC_NOTIFY)) {
            bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
            return BT_GATT_ITER_STOP;
        }
        subscription.value_handle = chrc->value_handle;
        uuid.val = BT_UUID_GATT_CCC_VAL;
        discovery.type = BT_GATT_DISCOVER_DESCRIPTOR;
        discovery.start_handle = chrc->value_handle + 1;
    } else {
        subscription.notify = notified;
        subscription.subscribe = subscribed;
        subscription.value = BT_GATT_CCC_NOTIFY;
        subscription.ccc_handle = attr->handle;
        err = bt_gatt_subscribe(conn, &subscription);
        if (err && err != -EALREADY) {
            LOG_ERR("Subscribe failed: %d", err);
            bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        } else LOG_INF("Heart rate notifications requested");
        return BT_GATT_ITER_STOP;
    }
    err = bt_gatt_discover(conn, &discovery);
    if (err) {
        LOG_ERR("Discovery failed: %d", err);
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
    return BT_GATT_ITER_STOP;
}
static void scan_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(scan_work, scan_work_handler);
static bool advertised(struct bt_data *data, void *context)
{
    bool *found = context;
    if (data->type == BT_DATA_UUID16_ALL || data->type == BT_DATA_UUID16_SOME) {
        for (size_t i = 0; i + 1 < data->data_len; i += 2) {
            if (sys_get_le16(data->data + i) == BT_UUID_HRS_VAL) *found = true;
        }
    }
    return true;
}
static void scan_result(const bt_addr_le_t *address, int8_t rssi, uint8_t type,
                        struct net_buf_simple *ad)
{
    ARG_UNUSED(rssi);
    if (peer || (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_SCAN_RSP)) return;
    bool found = false;
    bt_data_parse(ad, advertised, &found);
    if (!found || bt_le_scan_stop()) return;
    int err = bt_conn_le_create(address, BT_CONN_LE_CREATE_CONN,
                                BT_LE_CONN_PARAM_DEFAULT, &peer);
    if (err) {
        LOG_WRN("Connection creation failed: %d", err);
        k_work_reschedule(&scan_work, K_SECONDS(1));
    }
}
static void scan_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    if (peer) return;
    const struct bt_le_scan_param params = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW / 2,
    };
    int err = bt_le_scan_start(&params, scan_result);
    if (err && err != -EALREADY) k_work_reschedule(&scan_work, K_SECONDS(1));
    else LOG_INF("Scanning for BLE Heart Rate Service 180D");
}
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        if (peer) bt_conn_unref(peer);
        peer = NULL;
        k_work_reschedule(&scan_work, K_SECONDS(1));
        return;
    }
    LOG_INF("BLE connected; discovering heart rate service");
    k_work_reschedule(&receive_watchdog, K_SECONDS(15));
    memset(&subscription, 0, sizeof(subscription));
    memset(&discovery, 0, sizeof(discovery));
    uuid.val = BT_UUID_HRS_VAL;
    discovery.uuid = &uuid.uuid;
    discovery.func = discovered;
    discovery.type = BT_GATT_DISCOVER_PRIMARY;
    discovery.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discovery.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    if (bt_gatt_discover(conn, &discovery))
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(conn);
    LOG_INF("BLE disconnected: %u", reason);
    k_work_cancel_delayable(&receive_watchdog);
    invalidate();
    if (peer) bt_conn_unref(peer);
    peer = NULL;
    k_work_reschedule(&scan_work, K_SECONDS(1));
}
BT_CONN_CB_DEFINE(connection_callbacks) = {
    .connected = connected, .disconnected = disconnected,
};
static void ant_callback(ant_evt_t *event)
{
    if (k_msgq_put(&ant_events, event, K_NO_WAIT)) atomic_set(&ant_overflow, 1);
}
int main(void)
{
    int err = ant_init();
    if (!err) err = ant_cb_register(ant_callback);
    if (!err) err = ant_plus_key_set(0);
    if (!err) err = ant_hrm_sens_init(&hrm, HRM_SENS_CHANNEL_CONFIG(relay),
                                     HRM_SENS_PROFILE_CONFIG(relay));
    if (err) { LOG_ERR("ANT initialization failed: %d", err); return err; }
    hrm.HRM_PROFILE_manuf_id = 255;
    hrm.HRM_PROFILE_serial_num = 4242;
    hrm.HRM_PROFILE_hw_version = 1;
    hrm.HRM_PROFILE_sw_version = 1;
    hrm.HRM_PROFILE_model_num = 1;
    err = bt_enable(NULL);
    if (err) { LOG_ERR("BLE initialization failed: %d", err); return err; }
    k_work_reschedule(&scan_work, K_NO_WAIT);
    while (true) {
        ant_evt_t event;
        if (!k_msgq_get(&ant_events, &event, K_MSEC(50))) {
            if (event.event == EVENT_CHANNEL_CLOSED) {
                ant_open = false;
                ant_closing = false;
            }
            /* The profile callback runs after encoding; refresh beforehand too. */
            profile_event(&hrm, ANT_HRM_PAGE_0_UPDATED);
            ant_hrm_sens_evt_handler(&event, &hrm);
        }
        if (atomic_get(&ant_overflow)) { LOG_ERR("ANT event overflow"); k_oops(); }
        k_spinlock_key_t key = k_spin_lock(&sample_lock);
        bool fresh = sample.valid && k_uptime_get() - sample.received < STALE_MS;
        k_spin_unlock(&sample_lock, key);
        if (fresh && !ant_open && !ant_closing) {
            profile_event(&hrm, ANT_HRM_PAGE_0_UPDATED);
            err = ant_hrm_sens_open(&hrm);
            if (!err) { ant_open = true; LOG_INF("ANT+ HR started, device 4242"); }
            else { LOG_ERR("ANT open failed: %d", err); k_sleep(K_SECONDS(1)); }
        } else if (!fresh && ant_open && !ant_closing) {
            err = ant_channel_close(0);
            if (!err) { ant_closing = true; LOG_INF("ANT+ paused: no fresh heart rate"); }
            else { LOG_ERR("ANT close failed: %d", err); k_sleep(K_SECONDS(1)); }
        }
    }
}
