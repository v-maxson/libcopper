#include "unity.h"
#include <copper/copper.h>

#if defined(CPR_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)
#include <pthread.h>
#endif

void setUp(void)
{
}
void tearDown(void)
{
}

// --- Null guards ---

void test_init_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_rwlock_init(NULL));
}

void test_destroy_null(void)
{
	cpr_rwlock_destroy(NULL); /* must not crash */
}

void test_lckread_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_rwlock_lckread(NULL));
}

void test_try_lckread_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_rwlock_try_lckread(NULL));
}

void test_ulckread_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_rwlock_ulckread(NULL));
}

void test_lckwrite_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_rwlock_lckwrite(NULL));
}

void test_try_lckwrite_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_rwlock_try_lckwrite(NULL));
}

void test_ulckwrite_null(void)
{
	TEST_ASSERT_EQUAL_INT(CPR_ERR_INVALID, cpr_rwlock_ulckwrite(NULL));
}

// --- Lifecycle ---

void test_init_succeeds(void)
{
	CprRwLock rw;
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_init(&rw));
	cpr_rwlock_destroy(&rw);
}

// --- Single-threaded ---

void test_single_reader(void)
{
	CprRwLock rw;
	cpr_rwlock_init(&rw);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_lckread(&rw));
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_ulckread(&rw));
	cpr_rwlock_destroy(&rw);
}

void test_single_writer(void)
{
	CprRwLock rw;
	cpr_rwlock_init(&rw);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_lckwrite(&rw));
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_ulckwrite(&rw));
	cpr_rwlock_destroy(&rw);
}

void test_try_lckread_succeeds(void)
{
	CprRwLock rw;
	cpr_rwlock_init(&rw);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_try_lckread(&rw));
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_ulckread(&rw));
	cpr_rwlock_destroy(&rw);
}

void test_try_lckwrite_succeeds(void)
{
	CprRwLock rw;
	cpr_rwlock_init(&rw);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_try_lckwrite(&rw));
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_ulckwrite(&rw));
	cpr_rwlock_destroy(&rw);
}

// --- Multi-threaded ---

#define NREADERS 4
#define NWRITERS 4
#define WRITER_INCREMENTS 1000

typedef struct {
	CprRwLock    *rwlock;
	volatile int *counter;
	CprResult     result;
} WriterArgs;

typedef struct {
	CprRwLock  *rwlock;
	CprMutex   *coord;
	CprCondVar *cv;
	int	   *locked;
	int	   *release;
	CprResult   result;
} RwArgs;

typedef struct {
	CprRwLock  *rwlock;
	CprMutex   *coord;
	CprCondVar *cv;
	int	   *active;
	int	   *release;
	CprResult   result;
} ReaderArgs;

#if defined(CPR_PLATFORM_WINDOWS)

static DWORD WINAPI cpr__writer_counter_worker(LPVOID arg)
{
	WriterArgs *a = (WriterArgs *)arg;
	int i;
	for (i = 0; i < WRITER_INCREMENTS; i++) {
		CprResult r = cpr_rwlock_lckwrite(a->rwlock);
		if (cpr_err(r)) { a->result = r; return 0; }
		(*a->counter)++;
		r = cpr_rwlock_ulckwrite(a->rwlock);
		if (cpr_err(r)) { a->result = r; return 0; }
	}
	return 0;
}

static DWORD WINAPI cpr__write_worker(LPVOID arg)
{
	RwArgs *a = (RwArgs *)arg;
	a->result = cpr_rwlock_lckwrite(a->rwlock);
	if (cpr_err(a->result))
		return 0;
	cpr_mutex_lock(a->coord);
	*a->locked = 1;
	cpr_condvar_signal(a->cv);
	while (!*a->release)
		cpr_condvar_wait(a->cv, a->coord);
	cpr_mutex_unlock(a->coord);
	cpr_rwlock_ulckwrite(a->rwlock);
	return 0;
}

static DWORD WINAPI cpr__reader_worker(LPVOID arg)
{
	ReaderArgs *a = (ReaderArgs *)arg;
	a->result = cpr_rwlock_lckread(a->rwlock);
	if (cpr_err(a->result))
		return 0;
	cpr_mutex_lock(a->coord);
	(*a->active)++;
	cpr_condvar_broadcast(a->cv);
	while (!*a->release)
		cpr_condvar_wait(a->cv, a->coord);
	cpr_mutex_unlock(a->coord);
	cpr_rwlock_ulckread(a->rwlock);
	return 0;
}

void test_concurrent_readers(void)
{
	CprRwLock rw;
	CprMutex coord;
	CprCondVar cv;
	int active = 0, release = 0;
	HANDLE threads[NREADERS];
	ReaderArgs args[NREADERS];
	int i;

	cpr_rwlock_init(&rw);
	cpr_mutex_init(&coord);
	cpr_condvar_init(&cv);

	for (i = 0; i < NREADERS; i++) {
		args[i].rwlock	= &rw;
		args[i].coord	= &coord;
		args[i].cv	= &cv;
		args[i].active	= &active;
		args[i].release = &release;
		args[i].result	= CPR_OK;
		threads[i] = CreateThread(NULL, 0, cpr__reader_worker,
					  &args[i], 0, NULL);
	}

	cpr_mutex_lock(&coord);
	while (active < NREADERS)
		cpr_condvar_wait(&cv, &coord);
	TEST_ASSERT_EQUAL_INT(NREADERS, active);
	release = 1;
	cpr_condvar_broadcast(&cv);
	cpr_mutex_unlock(&coord);

	WaitForMultipleObjects(NREADERS, threads, TRUE, INFINITE);
	for (i = 0; i < NREADERS; i++)
		CloseHandle(threads[i]);
	for (i = 0; i < NREADERS; i++)
		TEST_ASSERT_EQUAL_INT(CPR_OK, args[i].result);

	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&coord);
	cpr_rwlock_destroy(&rw);
}

void test_writer_exclusive(void)
{
	CprRwLock rw;
	CprMutex coord;
	CprCondVar cv;
	int locked = 0, release = 0;
	HANDLE t;
	RwArgs args;

	cpr_rwlock_init(&rw);
	cpr_mutex_init(&coord);
	cpr_condvar_init(&cv);

	args.rwlock	= &rw;
	args.coord	= &coord;
	args.cv		= &cv;
	args.locked	= &locked;
	args.release	= &release;
	args.result	= CPR_OK;
	t = CreateThread(NULL, 0, cpr__write_worker, &args, 0, NULL);

	cpr_mutex_lock(&coord);
	while (!locked)
		cpr_condvar_wait(&cv, &coord);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_BUSY, cpr_rwlock_try_lckread(&rw));
	release = 1;
	cpr_condvar_signal(&cv);
	cpr_mutex_unlock(&coord);

	WaitForSingleObject(t, INFINITE);
	CloseHandle(t);

	TEST_ASSERT_EQUAL_INT(CPR_OK, args.result);
	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&coord);
	cpr_rwlock_destroy(&rw);
}

void test_try_lckwrite_busy(void)
{
	CprRwLock rw;
	CprMutex coord;
	CprCondVar cv;
	int active = 0, release = 0;
	HANDLE t;
	ReaderArgs args;

	cpr_rwlock_init(&rw);
	cpr_mutex_init(&coord);
	cpr_condvar_init(&cv);

	args.rwlock	= &rw;
	args.coord	= &coord;
	args.cv		= &cv;
	args.active	= &active;
	args.release	= &release;
	args.result	= CPR_OK;
	t = CreateThread(NULL, 0, cpr__reader_worker, &args, 0, NULL);

	cpr_mutex_lock(&coord);
	while (!active)
		cpr_condvar_wait(&cv, &coord);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_BUSY, cpr_rwlock_try_lckwrite(&rw));
	release = 1;
	cpr_condvar_broadcast(&cv);
	cpr_mutex_unlock(&coord);

	WaitForSingleObject(t, INFINITE);
	CloseHandle(t);

	TEST_ASSERT_EQUAL_INT(CPR_OK, args.result);
	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&coord);
	cpr_rwlock_destroy(&rw);
}

void test_write_then_read(void)
{
	CprRwLock rw;
	CprMutex coord;
	CprCondVar cv;
	int locked = 0, release = 0;
	HANDLE t;
	RwArgs args;

	cpr_rwlock_init(&rw);
	cpr_mutex_init(&coord);
	cpr_condvar_init(&cv);

	args.rwlock	= &rw;
	args.coord	= &coord;
	args.cv		= &cv;
	args.locked	= &locked;
	args.release	= &release;
	args.result	= CPR_OK;
	t = CreateThread(NULL, 0, cpr__write_worker, &args, 0, NULL);

	cpr_mutex_lock(&coord);
	while (!locked)
		cpr_condvar_wait(&cv, &coord);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_BUSY, cpr_rwlock_try_lckread(&rw));
	release = 1;
	cpr_condvar_signal(&cv);
	cpr_mutex_unlock(&coord);

	WaitForSingleObject(t, INFINITE);
	CloseHandle(t);

	TEST_ASSERT_EQUAL_INT(CPR_OK, args.result);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_lckread(&rw));
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_ulckread(&rw));

	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&coord);
	cpr_rwlock_destroy(&rw);
}

void test_write_contention_correctness(void)
{
	CprRwLock rw;
	volatile int counter = 0;
	HANDLE threads[NWRITERS];
	WriterArgs args[NWRITERS];
	int i;

	cpr_rwlock_init(&rw);
	for (i = 0; i < NWRITERS; i++) {
		args[i].rwlock	= &rw;
		args[i].counter = &counter;
		args[i].result	= CPR_OK;
		threads[i] = CreateThread(NULL, 0, cpr__writer_counter_worker,
					  &args[i], 0, NULL);
	}

	WaitForMultipleObjects(NWRITERS, threads, TRUE, INFINITE);
	for (i = 0; i < NWRITERS; i++)
		CloseHandle(threads[i]);

	for (i = 0; i < NWRITERS; i++)
		TEST_ASSERT_EQUAL_INT(CPR_OK, args[i].result);
	TEST_ASSERT_EQUAL_INT(NWRITERS * WRITER_INCREMENTS, (int)counter);
	cpr_rwlock_destroy(&rw);
}

#elif defined(CPR_PLATFORM_UNIX) || defined(CPR_PLATFORM_APPLE)

static void *cpr__writer_counter_worker(void *arg)
{
	WriterArgs *a = (WriterArgs *)arg;
	int i;
	for (i = 0; i < WRITER_INCREMENTS; i++) {
		CprResult r = cpr_rwlock_lckwrite(a->rwlock);
		if (cpr_err(r)) { a->result = r; return NULL; }
		(*a->counter)++;
		r = cpr_rwlock_ulckwrite(a->rwlock);
		if (cpr_err(r)) { a->result = r; return NULL; }
	}
	return NULL;
}

static void *cpr__write_worker(void *arg)
{
	RwArgs *a = (RwArgs *)arg;
	a->result = cpr_rwlock_lckwrite(a->rwlock);
	if (cpr_err(a->result))
		return NULL;
	cpr_mutex_lock(a->coord);
	*a->locked = 1;
	cpr_condvar_signal(a->cv);
	while (!*a->release)
		cpr_condvar_wait(a->cv, a->coord);
	cpr_mutex_unlock(a->coord);
	cpr_rwlock_ulckwrite(a->rwlock);
	return NULL;
}

static void *cpr__reader_worker(void *arg)
{
	ReaderArgs *a = (ReaderArgs *)arg;
	a->result = cpr_rwlock_lckread(a->rwlock);
	if (cpr_err(a->result))
		return NULL;
	cpr_mutex_lock(a->coord);
	(*a->active)++;
	cpr_condvar_broadcast(a->cv);
	while (!*a->release)
		cpr_condvar_wait(a->cv, a->coord);
	cpr_mutex_unlock(a->coord);
	cpr_rwlock_ulckread(a->rwlock);
	return NULL;
}

void test_concurrent_readers(void)
{
	CprRwLock rw;
	CprMutex coord;
	CprCondVar cv;
	int active = 0, release = 0;
	pthread_t threads[NREADERS];
	ReaderArgs args[NREADERS];
	int i;

	cpr_rwlock_init(&rw);
	cpr_mutex_init(&coord);
	cpr_condvar_init(&cv);

	for (i = 0; i < NREADERS; i++) {
		args[i].rwlock	= &rw;
		args[i].coord	= &coord;
		args[i].cv	= &cv;
		args[i].active	= &active;
		args[i].release = &release;
		args[i].result	= CPR_OK;
		pthread_create(&threads[i], NULL, cpr__reader_worker, &args[i]);
	}

	cpr_mutex_lock(&coord);
	while (active < NREADERS)
		cpr_condvar_wait(&cv, &coord);
	TEST_ASSERT_EQUAL_INT(NREADERS, active);
	release = 1;
	cpr_condvar_broadcast(&cv);
	cpr_mutex_unlock(&coord);

	for (i = 0; i < NREADERS; i++)
		pthread_join(threads[i], NULL);
	for (i = 0; i < NREADERS; i++)
		TEST_ASSERT_EQUAL_INT(CPR_OK, args[i].result);

	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&coord);
	cpr_rwlock_destroy(&rw);
}

void test_writer_exclusive(void)
{
	CprRwLock rw;
	CprMutex coord;
	CprCondVar cv;
	int locked = 0, release = 0;
	pthread_t t;
	RwArgs args;

	cpr_rwlock_init(&rw);
	cpr_mutex_init(&coord);
	cpr_condvar_init(&cv);

	args.rwlock	= &rw;
	args.coord	= &coord;
	args.cv		= &cv;
	args.locked	= &locked;
	args.release	= &release;
	args.result	= CPR_OK;
	pthread_create(&t, NULL, cpr__write_worker, &args);

	cpr_mutex_lock(&coord);
	while (!locked)
		cpr_condvar_wait(&cv, &coord);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_BUSY, cpr_rwlock_try_lckread(&rw));
	release = 1;
	cpr_condvar_signal(&cv);
	cpr_mutex_unlock(&coord);

	pthread_join(t, NULL);

	TEST_ASSERT_EQUAL_INT(CPR_OK, args.result);
	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&coord);
	cpr_rwlock_destroy(&rw);
}

void test_try_lckwrite_busy(void)
{
	CprRwLock rw;
	CprMutex coord;
	CprCondVar cv;
	int active = 0, release = 0;
	pthread_t t;
	ReaderArgs args;

	cpr_rwlock_init(&rw);
	cpr_mutex_init(&coord);
	cpr_condvar_init(&cv);

	args.rwlock	= &rw;
	args.coord	= &coord;
	args.cv		= &cv;
	args.active	= &active;
	args.release	= &release;
	args.result	= CPR_OK;
	pthread_create(&t, NULL, cpr__reader_worker, &args);

	cpr_mutex_lock(&coord);
	while (!active)
		cpr_condvar_wait(&cv, &coord);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_BUSY, cpr_rwlock_try_lckwrite(&rw));
	release = 1;
	cpr_condvar_broadcast(&cv);
	cpr_mutex_unlock(&coord);

	pthread_join(t, NULL);

	TEST_ASSERT_EQUAL_INT(CPR_OK, args.result);
	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&coord);
	cpr_rwlock_destroy(&rw);
}

void test_write_then_read(void)
{
	CprRwLock rw;
	CprMutex coord;
	CprCondVar cv;
	int locked = 0, release = 0;
	pthread_t t;
	RwArgs args;

	cpr_rwlock_init(&rw);
	cpr_mutex_init(&coord);
	cpr_condvar_init(&cv);

	args.rwlock	= &rw;
	args.coord	= &coord;
	args.cv		= &cv;
	args.locked	= &locked;
	args.release	= &release;
	args.result	= CPR_OK;
	pthread_create(&t, NULL, cpr__write_worker, &args);

	cpr_mutex_lock(&coord);
	while (!locked)
		cpr_condvar_wait(&cv, &coord);
	TEST_ASSERT_EQUAL_INT(CPR_ERR_BUSY, cpr_rwlock_try_lckread(&rw));
	release = 1;
	cpr_condvar_signal(&cv);
	cpr_mutex_unlock(&coord);

	pthread_join(t, NULL);

	TEST_ASSERT_EQUAL_INT(CPR_OK, args.result);
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_lckread(&rw));
	TEST_ASSERT_EQUAL_INT(CPR_OK, cpr_rwlock_ulckread(&rw));

	cpr_condvar_destroy(&cv);
	cpr_mutex_destroy(&coord);
	cpr_rwlock_destroy(&rw);
}

void test_write_contention_correctness(void)
{
	CprRwLock rw;
	volatile int counter = 0;
	pthread_t threads[NWRITERS];
	WriterArgs args[NWRITERS];
	int i;

	cpr_rwlock_init(&rw);
	for (i = 0; i < NWRITERS; i++) {
		args[i].rwlock	= &rw;
		args[i].counter = &counter;
		args[i].result	= CPR_OK;
		pthread_create(&threads[i], NULL, cpr__writer_counter_worker,
			       &args[i]);
	}

	for (i = 0; i < NWRITERS; i++)
		pthread_join(threads[i], NULL);

	for (i = 0; i < NWRITERS; i++)
		TEST_ASSERT_EQUAL_INT(CPR_OK, args[i].result);
	TEST_ASSERT_EQUAL_INT(NWRITERS * WRITER_INCREMENTS, (int)counter);
	cpr_rwlock_destroy(&rw);
}

#endif /* threading */

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_init_null);
	RUN_TEST(test_destroy_null);
	RUN_TEST(test_lckread_null);
	RUN_TEST(test_try_lckread_null);
	RUN_TEST(test_ulckread_null);
	RUN_TEST(test_lckwrite_null);
	RUN_TEST(test_try_lckwrite_null);
	RUN_TEST(test_ulckwrite_null);

	RUN_TEST(test_init_succeeds);

	RUN_TEST(test_single_reader);
	RUN_TEST(test_single_writer);
	RUN_TEST(test_try_lckread_succeeds);
	RUN_TEST(test_try_lckwrite_succeeds);

#if defined(CPR_PLATFORM_WINDOWS) || defined(CPR_PLATFORM_UNIX) || \
	defined(CPR_PLATFORM_APPLE)
	RUN_TEST(test_concurrent_readers);
	RUN_TEST(test_writer_exclusive);
	RUN_TEST(test_try_lckwrite_busy);
	RUN_TEST(test_write_then_read);
	RUN_TEST(test_write_contention_correctness);
#endif

	return UNITY_END();
}
