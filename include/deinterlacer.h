/*
	Initialize and store deinterlacer settings.
*/
void deinterlacer_init(
	unsigned short iw,
	unsigned short ih,
	unsigned char corr,
	unsigned char bias);

/*
	Allocate memory for all the working buffers.
	@return 0 on success
			1 if the settings are uninitialized or one of the allocations failed.
*/
int deinterlacer_create(void);

/*
	Deallocate memory from deinterlacer_create()
*/
void deinterlacer_destroy(void);


/*
	@in_parameters
	- in_data[0] : start address of y
	- in_data[1] : start address of u
	- in_data[2] : start address of v
	- in_stride[0] : y stride
	- in_stride[1] : u stride
	- in_stride[2] : v stride

	@out_parameters
	- out_data[0] : start address of y
	- out _data[1] : start address of u
	- out _data[2] : start address of v
	- out _stride[0] : y stride
	- out _stride[1] : u stride
	- out _stride[2] : v stride
*/
void deinterlacer_run(unsigned char *in_data[3],
	int in_stride[3],
	unsigned char *out_data[3],
	int out_stride[3]);
