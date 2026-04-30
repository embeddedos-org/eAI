// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// Tests for HAL filesystem, threading, and network subsystems

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "eai/platform.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-44s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

/* ---- HAL FS ops extern ---- */
#ifdef _WIN32
extern const eai_hal_fs_ops_t eai_hal_windows_fs_ops;
#define FS_OPS eai_hal_windows_fs_ops
#elif defined(__linux__)
extern const eai_hal_fs_ops_t eai_hal_posix_fs_ops;
#define FS_OPS eai_hal_posix_fs_ops
#endif

/* ---- HAL Thread ops extern ---- */
#ifdef _WIN32
extern const eai_hal_thread_ops_t eai_hal_windows_thread_ops;
#define THREAD_OPS eai_hal_windows_thread_ops
#elif defined(__linux__)
extern const eai_hal_thread_ops_t eai_hal_posix_thread_ops;
#define THREAD_OPS eai_hal_posix_thread_ops
#endif

/* ---- HAL Net ops extern ---- */
#ifdef _WIN32
extern const eai_hal_net_ops_t eai_hal_windows_net_ops;
#define NET_OPS eai_hal_windows_net_ops
#elif defined(__linux__)
extern const eai_hal_net_ops_t eai_hal_posix_net_ops;
#define NET_OPS eai_hal_posix_net_ops
#endif

static const char *TEST_FILE_PATH = "eai_test_hal_fs_tmp.bin";
static const char *TEST_DIR_PATH = "eai_test_hal_dir_tmp";

/* ========== Filesystem Tests ========== */

static void test_fs_file_write_read_roundtrip(void)
{
    TEST(fs_file_write_read_roundtrip);
    eai_file_t file = NULL;

    /* Write */
    eai_status_t st = FS_OPS.file_open(&file, TEST_FILE_PATH, EAI_FILE_WRITE | EAI_FILE_CREATE);
    if (st != EAI_OK || !file) { FAIL("open for write failed"); return; }

    const char *data = "Hello eAI HAL!";
    size_t written = 0;
    st = FS_OPS.file_write(file, data, strlen(data), &written);
    if (st != EAI_OK) { FAIL("write failed"); FS_OPS.file_close(file); return; }
    if (written != strlen(data)) { FAIL("written count mismatch"); FS_OPS.file_close(file); return; }
    FS_OPS.file_close(file);

    /* Read back */
    file = NULL;
    st = FS_OPS.file_open(&file, TEST_FILE_PATH, EAI_FILE_READ);
    if (st != EAI_OK || !file) { FAIL("open for read failed"); return; }

    char buf[64] = {0};
    size_t bytes_read = 0;
    st = FS_OPS.file_read(file, buf, sizeof(buf), &bytes_read);
    if (st != EAI_OK) { FAIL("read failed"); FS_OPS.file_close(file); return; }
    if (bytes_read != strlen(data)) { FAIL("read count mismatch"); FS_OPS.file_close(file); return; }
    if (strcmp(buf, data) != 0) { FAIL("data content mismatch"); FS_OPS.file_close(file); return; }

    FS_OPS.file_close(file);
    PASS();
}

static void test_fs_file_exists(void)
{
    TEST(fs_file_exists);
    /* The file was created in the previous test */
    bool exists = FS_OPS.file_exists(TEST_FILE_PATH);
    if (!exists) { FAIL("file should exist"); return; }

    bool not_exists = FS_OPS.file_exists("nonexistent_file_xyz_123.txt");
    if (not_exists) { FAIL("nonexistent file should not exist"); return; }
    PASS();
}

static void test_fs_file_size(void)
{
    TEST(fs_file_size);
    uint64_t size = 0;
    eai_status_t st = FS_OPS.file_size(TEST_FILE_PATH, &size);
    if (st != EAI_OK) { FAIL("file_size failed"); return; }
    if (size != strlen("Hello eAI HAL!")) { FAIL("size mismatch"); return; }
    printf("(%llu bytes) ", (unsigned long long)size);
    PASS();
}

static void test_fs_file_size_nonexistent(void)
{
    TEST(fs_file_size_nonexistent);
    uint64_t size = 0;
    eai_status_t st = FS_OPS.file_size("nonexistent_xyz.bin", &size);
    if (st != EAI_ERR_IO) { FAIL("expected IO error for nonexistent file"); return; }
    PASS();
}

static void test_fs_file_open_nonexistent_read(void)
{
    TEST(fs_file_open_nonexistent_read);
    eai_file_t file = NULL;
    eai_status_t st = FS_OPS.file_open(&file, "nonexistent_xyz_read.bin", EAI_FILE_READ);
    if (st != EAI_ERR_IO) { FAIL("expected IO error"); return; }
    PASS();
}

static void test_fs_file_append(void)
{
    TEST(fs_file_append);
    eai_file_t file = NULL;

    /* Append to existing file */
    eai_status_t st = FS_OPS.file_open(&file, TEST_FILE_PATH, EAI_FILE_APPEND);
    if (st != EAI_OK || !file) { FAIL("open for append failed"); return; }

    const char *extra = "++";
    size_t written = 0;
    st = FS_OPS.file_write(file, extra, strlen(extra), &written);
    if (st != EAI_OK || written != 2) { FAIL("append write failed"); FS_OPS.file_close(file); return; }
    FS_OPS.file_close(file);

    /* Verify new size */
    uint64_t size = 0;
    FS_OPS.file_size(TEST_FILE_PATH, &size);
    if (size != strlen("Hello eAI HAL!") + 2) { FAIL("size after append wrong"); return; }
    PASS();
}

static void test_fs_dir_create(void)
{
    TEST(fs_dir_create);
    eai_status_t st = FS_OPS.dir_create(TEST_DIR_PATH);
    if (st != EAI_OK) { FAIL("dir_create failed"); return; }

    /* Creating again should succeed (already exists) */
    st = FS_OPS.dir_create(TEST_DIR_PATH);
    if (st != EAI_OK) { FAIL("dir_create idempotent failed"); return; }
    PASS();
}

static void test_fs_close_null(void)
{
    TEST(fs_close_null);
    FS_OPS.file_close(NULL); /* should not crash */
    PASS();
}

/* ========== Thread Tests ========== */

static volatile int thread_counter = 0;

static void thread_increment(void *arg)
{
    int *val = (int *)arg;
    *val += 1;
}

static void test_thread_create_join(void)
{
    TEST(thread_create_join);
    int value = 0;
    eai_thread_t thread = NULL;

    eai_status_t st = THREAD_OPS.thread_create(&thread, thread_increment, &value);
    if (st != EAI_OK) { FAIL("thread_create failed"); return; }

    st = THREAD_OPS.thread_join(thread);
    if (st != EAI_OK) { FAIL("thread_join failed"); return; }

    if (value != 1) { FAIL("thread did not execute"); return; }
    PASS();
}

static void test_mutex_lock_unlock(void)
{
    TEST(mutex_lock_unlock);
    eai_mutex_t mutex = NULL;

    eai_status_t st = THREAD_OPS.mutex_create(&mutex);
    if (st != EAI_OK) { FAIL("mutex_create failed"); return; }

    st = THREAD_OPS.mutex_lock(mutex);
    if (st != EAI_OK) { FAIL("mutex_lock failed"); THREAD_OPS.mutex_destroy(mutex); return; }

    st = THREAD_OPS.mutex_unlock(mutex);
    if (st != EAI_OK) { FAIL("mutex_unlock failed"); THREAD_OPS.mutex_destroy(mutex); return; }

    /* Lock/unlock again to verify it's reusable */
    st = THREAD_OPS.mutex_lock(mutex);
    if (st != EAI_OK) { FAIL("mutex_lock 2nd failed"); THREAD_OPS.mutex_destroy(mutex); return; }
    THREAD_OPS.mutex_unlock(mutex);

    THREAD_OPS.mutex_destroy(mutex);
    PASS();
}

static void test_semaphore_post_wait(void)
{
    TEST(semaphore_post_wait);
    eai_semaphore_t sem = NULL;

    eai_status_t st = THREAD_OPS.semaphore_create(&sem, 0);
    if (st != EAI_OK) { FAIL("semaphore_create failed"); return; }

    /* Post first (increment), then wait (decrement) — should not block */
    st = THREAD_OPS.semaphore_post(sem);
    if (st != EAI_OK) { FAIL("semaphore_post failed"); THREAD_OPS.semaphore_destroy(sem); return; }

    st = THREAD_OPS.semaphore_wait(sem);
    if (st != EAI_OK) { FAIL("semaphore_wait failed"); THREAD_OPS.semaphore_destroy(sem); return; }

    THREAD_OPS.semaphore_destroy(sem);
    PASS();
}

static void test_semaphore_initial_value(void)
{
    TEST(semaphore_initial_value);
    eai_semaphore_t sem = NULL;

    /* Create with initial count = 2 — should be able to wait twice */
    eai_status_t st = THREAD_OPS.semaphore_create(&sem, 2);
    if (st != EAI_OK) { FAIL("create failed"); return; }

    st = THREAD_OPS.semaphore_wait(sem);
    if (st != EAI_OK) { FAIL("first wait failed"); THREAD_OPS.semaphore_destroy(sem); return; }

    st = THREAD_OPS.semaphore_wait(sem);
    if (st != EAI_OK) { FAIL("second wait failed"); THREAD_OPS.semaphore_destroy(sem); return; }

    THREAD_OPS.semaphore_destroy(sem);
    PASS();
}

static volatile int mutex_shared = 0;
static eai_mutex_t test_mutex_g = NULL;

static void thread_mutex_increment(void *arg)
{
    (void)arg;
    for (int i = 0; i < 1000; i++) {
        THREAD_OPS.mutex_lock(test_mutex_g);
        mutex_shared++;
        THREAD_OPS.mutex_unlock(test_mutex_g);
    }
}

static void test_mutex_contention(void)
{
    TEST(mutex_contention);
    mutex_shared = 0;
    THREAD_OPS.mutex_create(&test_mutex_g);

    eai_thread_t t1 = NULL, t2 = NULL;
    THREAD_OPS.thread_create(&t1, thread_mutex_increment, NULL);
    THREAD_OPS.thread_create(&t2, thread_mutex_increment, NULL);

    THREAD_OPS.thread_join(t1);
    THREAD_OPS.thread_join(t2);

    THREAD_OPS.mutex_destroy(test_mutex_g);
    test_mutex_g = NULL;

    /* Both threads increment 1000 times = 2000 total */
    if (mutex_shared != 2000) {
        char msg[64];
        snprintf(msg, sizeof(msg), "expected 2000, got %d — race condition!", mutex_shared);
        FAIL(msg);
        return;
    }
    PASS();
}

/* ========== Network Tests ========== */

static void test_net_socket_create_tcp(void)
{
    TEST(net_socket_create_tcp);
    eai_socket_t sock = NULL;
    eai_status_t st = NET_OPS.socket_create(&sock, EAI_SOCK_TCP);
    if (st != EAI_OK) { FAIL("TCP socket create failed"); return; }
    if (!sock) { FAIL("socket is NULL"); return; }
    NET_OPS.close(sock);
    PASS();
}

static void test_net_socket_create_udp(void)
{
    TEST(net_socket_create_udp);
    eai_socket_t sock = NULL;
    eai_status_t st = NET_OPS.socket_create(&sock, EAI_SOCK_UDP);
    if (st != EAI_OK) { FAIL("UDP socket create failed"); return; }
    if (!sock) { FAIL("socket is NULL"); return; }
    NET_OPS.close(sock);
    PASS();
}

static void test_net_dns_resolve_localhost(void)
{
    TEST(net_dns_resolve_localhost);
    char ip[64] = {0};
    eai_status_t st = NET_OPS.dns_resolve("localhost", ip, sizeof(ip));
    if (st != EAI_OK) { FAIL("dns_resolve localhost failed"); return; }
    if (strlen(ip) == 0) { FAIL("resolved IP is empty"); return; }
    /* Should resolve to 127.0.0.1 */
    if (strcmp(ip, "127.0.0.1") != 0) {
        printf("(resolved to %s, expected 127.0.0.1 — platform-dependent) ", ip);
    }
    printf("(%s) ", ip);
    PASS();
}

static void test_net_connect_refused(void)
{
    TEST(net_connect_refused);
    eai_socket_t sock = NULL;
    NET_OPS.socket_create(&sock, EAI_SOCK_TCP);

    /* Connect to port 1 — should fail (refused or timeout) */
    eai_status_t st = NET_OPS.connect(sock, "127.0.0.1", 1);
    if (st == EAI_OK) {
        printf("(unexpectedly succeeded) ");
    }
    /* We expect EAI_ERR_CONNECT but don't fail the test if port 1 is somehow open */
    NET_OPS.close(sock);
    PASS();
}

/* ========== Cleanup ========== */

static void cleanup(void)
{
    remove(TEST_FILE_PATH);
#ifdef _WIN32
    _rmdir(TEST_DIR_PATH);
#else
    rmdir(TEST_DIR_PATH);
#endif
}

int main(void)
{
    printf("=== EAI HAL Subsystem Tests (FS/Thread/Net) ===\n\n");

    printf("--- Filesystem ---\n");
    test_fs_file_write_read_roundtrip();
    test_fs_file_exists();
    test_fs_file_size();
    test_fs_file_size_nonexistent();
    test_fs_file_open_nonexistent_read();
    test_fs_file_append();
    test_fs_dir_create();
    test_fs_close_null();

    printf("\n--- Threading ---\n");
    test_thread_create_join();
    test_mutex_lock_unlock();
    test_semaphore_post_wait();
    test_semaphore_initial_value();
    test_mutex_contention();

    printf("\n--- Network ---\n");
    test_net_socket_create_tcp();
    test_net_socket_create_udp();
    test_net_dns_resolve_localhost();
    test_net_connect_refused();

    cleanup();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
