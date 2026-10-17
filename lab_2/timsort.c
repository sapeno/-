#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void insertion_sort(int *arr, int l, int r) {
    for (int i = l + 1; i <= r; i++) {
        int tmp = arr[i];
        int j = i - 1;

        while (j >= l && arr[j] > tmp){
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = tmp;
    }
}

void merge(int *arr, int l, int m, int r) {
    int len1 = m - l + 1;
    int len2 = r - m;
    int *L = (int*)malloc(len1 * sizeof(int));
    int *R = (int*)malloc(len2 * sizeof(int));

    for (int i = 0; i < len1; i++) L[i] = arr[l + i];
    for (int j = 0; j < len2; j++) R[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = l;
    while (i < len1 && j < len2) {
        if (L[i] <= R[j]) arr[k++] = L[i++];
        else arr[k++] = R[j++];
    }
    while (i < len1) arr[k++] = L[i++];
    while (j < len2) arr[k++] = R[j++];

    free(L);
    free(R);
}

void timsort_seq(int *arr, int n) {
    int run_size = 32; 

    for (int i = 0; i < n; i += run_size) {
        int right = (i + run_size - 1 < n - 1) ? (i + run_size - 1) : (n - 1);
        insertion_sort(arr, i, right);
    }

    for (int size = run_size; size < n; size = 2 * size) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = (left + 2 * size - 1 < n - 1) ? (left + 2 * size - 1) : (n - 1);
            if (mid < right) {
                merge(arr, left, mid, right);
            }
        }
    }
}

typedef struct {
    int *array;
    int left;
    int mid;
    int right;
} Task;

typedef struct {
    Task *tasks;
    int capacity;
    int head;
    int tail;
    int count;
    int active_threads;
    
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_cond_t all_done;
    
    int shutdown;
} ThreadPool;


void* worker_thread(void* arg) {
    ThreadPool *pool = (ThreadPool*)arg;
    
    while(1) {
        pthread_mutex_lock(&pool->lock);
        while (pool->count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->not_empty, &pool->lock);
        }
        if (pool->shutdown && pool->count == 0) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        Task task = pool->tasks[pool->head];
        pool->head = (pool->head + 1) % pool->capacity;
        pool->count--;
        pool->active_threads++;

        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->lock);

        merge(task.array, task.left, task.mid, task.right);

        pthread_mutex_lock(&pool->lock);
        pool->active_threads--;
        if (pool->active_threads == 0 && pool->count == 0) {
            pthread_cond_signal(&pool->all_done);
        }
        pthread_mutex_unlock(&pool->lock);
    }
    
    return NULL;
}

void print_os_threads() {
    char command[256];
    sprintf(command, "ps -o nlwp= -p %d", getpid());
    printf("Количество потоков в процессе сейчас: ");
    fflush(stdout);
    system(command);
}

ThreadPool *create_pool(int num_threads, int queue_capacity) {
    ThreadPool *pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    pool->tasks = (Task*)malloc(queue_capacity * sizeof(Task));
    pool->capacity = queue_capacity;
    pool->head = 0;
    pool->tail = 0;
    pool->count = 0;
    pool->active_threads = 0;
    pool->shutdown = 0;

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->not_empty, NULL);
    pthread_cond_init(&pool->not_full, NULL);
    pthread_cond_init(&pool->all_done, NULL);

    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, pool);
        pthread_detach(threads[i]);
    }

    print_os_threads();

    free(threads);
    return pool;
}

void add_task(ThreadPool *pool, int *arr, int l, int m, int r) {
    pthread_mutex_lock(&pool->lock);
    while (pool->count == pool->capacity && !pool->shutdown) {
        pthread_cond_wait(&pool->not_full, &pool->lock);
    }
    pool->tasks[pool->tail].array = arr;
    pool->tasks[pool->tail].left = l;
    pool->tasks[pool->tail].mid = m;
    pool->tasks[pool->tail].right = r;
    pool->tail = (pool->tail + 1) % pool->capacity;
    pool->count++;
    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
}

void wait_for_completion(ThreadPool *pool) {
    pthread_mutex_lock(&pool->lock);
    while (pool->count > 0 || pool->active_threads > 0) {
        pthread_cond_wait(&pool->all_done, &pool->lock);
    }
    pthread_mutex_unlock(&pool->lock);
}

void destroy_pool(ThreadPool *pool) {
    pthread_mutex_lock(&pool->lock);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
    free(pool->tasks);
    free(pool);
}

void timsort_parallel(int *arr, int n, int num_threads) {
    int runs_size = 32;
    for (int i = 0; i < n; i += runs_size) {
        int right = (i + runs_size - 1 < n -1) ? (i + runs_size - 1) : (n - 1);
        insertion_sort(arr,i, right);
    }

    ThreadPool *pool = create_pool(num_threads, 1024);

    for (int size = runs_size; size < n; size = 2*size) {
        for (int l = 0; l < n; l += 2*size) {
            int m = l + size - 1;
            int r = (l + 2*size - 1 < n - 1) ? (l + 2*size -1) : (n -1);

            if(m < r) add_task(pool, arr, l, m, r);
        }
        wait_for_completion(pool);
    }
    destroy_pool(pool);
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int is_sorted(int *arr, int n) {
    for (int i = 1; i < n; i++) {
        if (arr[i] < arr[i-1]) return 0;
    }
    return 1;
}



int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("\n");
    }

    int n = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    char *mode = argv[3];

    if (n <= 0) { 
        printf("размер массива должен быть > 0\n"); 
        return 1;
    }
    if (num_threads <= 0) {
        printf("количество потоков должно быть > 0\n");
        return 1;
    }

    int *arr = (int*)malloc(n * sizeof(int));
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 1000000;
    }

    double start_time = get_time();

     if (strcmp(mode, "seq") == 0) {
        printf("ПОСЛЕДОВАТЕЛЬНЫЙ\n");
        timsort_seq(arr, n);
    } else if (strcmp(mode, "par") == 0) {
        printf("ПАРАЛЛЕЛЬНЫЙ\n");
        printf("Запрошено потоков: %d\n", num_threads);
        timsort_parallel(arr, n, num_threads);
    } else {
        printf("Неизвестный режим: %s\n", mode);
        free(arr);
        return 1;
    }
    
    double end_time = get_time();
    double duration = end_time - start_time;

    int sorted = is_sorted(arr, n);

    printf("\n");
    printf("Время выполнения: %.4f сек.\n", duration);
    printf("Массив отсортирован: %s\n", sorted ? "ДА" : "НЕТ");

    free(arr);
    return 0;
}

