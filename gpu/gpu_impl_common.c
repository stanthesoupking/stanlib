#define GPU_LOGGING 1
#define GPU_VALIDATION 0

#if GPU_LOGGING
	#define gpu_log(fmt, ...) \
		printf("[Gpu]: " fmt "\n", ##__VA_ARGS__)
#else
	#define gpu_log(fmt, ...) ((void)0)
#endif

#if GPU_VALIDATION
	#define gpu_validate(condition, message) sl_assert(condition, message)
#else
	#define gpu_validate(condition, message) ((void)0)
#endif

typedef struct Gpu_Callback {
	void* ctx;
	Gpu_Callback_Fn fn;
} Gpu_Callback;
sl_seq(Gpu_Callback, Gpu_Callback_Seq, gpu_callback_seq);

typedef struct Gpu_Semaphore_On_Notify_Callback {
	u64 value;
	void* ctx;
	Gpu_Callback_Fn fn;
} Gpu_Semaphore_On_Notify_Callback;
sl_seq(Gpu_Semaphore_On_Notify_Callback, Gpu_Semaphore_On_Notify_Callback_Seq, gpu_semaphore_on_notify_callback_seq);

typedef enum Gpu_Command_Buffer_State {
	Gpu_Command_Buffer_State_Idle,
	Gpu_Command_Buffer_State_Recording,
	Gpu_Command_Buffer_State_Enqueued
} Gpu_Command_Buffer_State;
