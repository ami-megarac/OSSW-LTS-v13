/****************************************************************************/
/*																			*/
/*	Module:			jbistub.c												*/
/*																			*/
/*					Copyright (C) Altera Corporation 1997-2001				*/
/*																			*/
/*	Description:	Jam STAPL ByteCode Player main source file				*/
/*																			*/
/*					Supports Altera ByteBlaster hardware download cable		*/
/*					on Windows 95 and Windows NT operating systems.			*/
/*					(A device driver is required for Windows NT.)			*/
/*																			*/
/*					Also supports BitBlaster hardware download cable on		*/
/*					Windows 95, Windows NT, and UNIX platforms.				*/
/*																			*/
/*	Revisions:		1.1 fixed control port initialization for ByteBlaster	*/
/*					2.0 added support for STAPL bytecode format, added code	*/
/*						to get printer port address from Windows registry	*/
/*					2.1 improved messages, fixed delay-calibration bug in	*/
/*						16-bit DOS port, added support for "alternative		*/
/*						cable X", added option to control whether to reset	*/
/*						the TAP after execution, moved porting macros into	*/
/*						jbiport.h											*/
/*					2.2 added support for static memory						*/
/*						fixed /W4 warnings									*/
/*																			*/
/****************************************************************************/

#include "jbiexprt.h"
#include "jtag_ioctl.h"

extern int jtag_io(int pin,int act);

typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
#define TRUE 1
#define FALSE 0
#define POINTER_ALIGNMENT sizeof(BYTE)
#define set_jtag_io jtag_io
#define read_jtag_io jtag_io
#define TDI_PIN 56
#define TDO_PIN	57
#define TCK_PIN	58
#define TMS_PIN	59
#define OPTIONALT_FUN "do_real_time_isp=1"

typedef enum {
	SET_GPIO_LOW = 0,
	SET_GPIO_HIGH,
	READ_GPIO_VAL,
	
	SET_GPIO_DIR_IN,
	SET_GPIO_DIR_OUT,
}eGpioactions;

int fg_Data_Readback = 0;
unsigned int *pReadData= NULL;

static int Pin_ctrl(int gpioNum, int action); 


/*
*
*	Global variables
*/

/* file buffer for Jam STAPL ByteCode input file */
unsigned char *file_buffer = NULL;

long file_length = 0L;

/* delay count for one millisecond delay */
long one_ms_delay = 0L;

/* delay count to reduce the maximum TCK frequency */
int tck_delay =0;

BOOL jtag_hardware_initialized = FALSE;

void initialize_jtag_hardware(void);
void close_jtag_hardware(void);

/* function prototypes to allow forward reference */
extern void delay_loop(long count);
/*
*
*	Customized interface functions for Jam STAPL ByteCode Player I/O:
*
*	jbi_jtag_io()
*	jbi_message()
*	jbi_delay()
*/

int jbi_jtag_io(int tms, int tdi, int read_tdo)
{
	int data __attribute__((unused)) = 0;
	int tdo __attribute__((unused)) = 0;
	int i __attribute__((unused)) = 0;
	int result __attribute__((unused)) = 0;
	char ch_data __attribute__((unused)) = 0;

	if (!jtag_hardware_initialized)
	{
		initialize_jtag_hardware();
		jtag_hardware_initialized = TRUE;
	}
	
    //write first (clock low and set other signals.)
    Pin_ctrl(TCK_PIN, 0); 
    Pin_ctrl(TDI_PIN, (tdi?1:0));
    Pin_ctrl(TMS_PIN, (tms?1:0));

    //read_tdo
    if (read_tdo){
        tdo = Pin_ctrl(TDO_PIN, READ_GPIO_VAL);	
    }
    //write second , clock H and clock low  

    Pin_ctrl(TCK_PIN, 1);
    Pin_ctrl(TCK_PIN, 0);

	return (tdo);
}

void jbi_message(char *message_text)
{
    /**** Try to get data back*****/
    if(fg_Data_Readback != 0){
        char* pStr = NULL;    
        pStr=strstr (message_text,"CODE");
        if (pStr != NULL){
            if(sscanf(pStr,"CODE is %X",pReadData)== 1);
            else if(sscanf(pStr,"CODE code is %X",pReadData)== 1);
        }
    }
    else {
        dbgprintf("%s.\n",message_text);
    }
}

void jbi_delay(long microseconds)
{
	delay_loop(microseconds *
		((one_ms_delay / 1000L) + ((one_ms_delay % 1000L) ? 1 : 0)));
}


void *jbi_malloc(unsigned int size)
{
	unsigned int n_bytes_to_allocate = (POINTER_ALIGNMENT * ((size + POINTER_ALIGNMENT - 1) / POINTER_ALIGNMENT));

	unsigned char *ptr = 0;

	ptr = (unsigned char *) kmalloc(n_bytes_to_allocate, GFP_DMA|GFP_KERNEL);

	return ptr;
}

void jbi_free(void *ptr)
{
	if(ptr != 0)
	{
		unsigned char *tmp_ptr = (unsigned char *) ptr;
		ptr = tmp_ptr;
		
		kfree(ptr);
	}
}


void calibrate_delay(void)
{
	int sample __attribute__((unused)) = 0;
	int count __attribute__((unused)) = 0;
	DWORD tick_count1 __attribute__((unused)) = 0L;
	DWORD tick_count2 __attribute__((unused)) = 0L;

	one_ms_delay = 0L;
	/* This is system-dependent!  Update this number for target system */
	one_ms_delay = 1000L;
}

char *error_text[] =
{
/* JBIC_SUCCESS            0 */ "success",
/* JBIC_OUT_OF_MEMORY      1 */ "out of memory",
/* JBIC_IO_ERROR           2 */ "file access error",
/* JAMC_SYNTAX_ERROR       3 */ "syntax error",
/* JBIC_UNEXPECTED_END     4 */ "unexpected end of file",
/* JBIC_UNDEFINED_SYMBOL   5 */ "undefined symbol",
/* JAMC_REDEFINED_SYMBOL   6 */ "redefined symbol",
/* JBIC_INTEGER_OVERFLOW   7 */ "integer overflow",
/* JBIC_DIVIDE_BY_ZERO     8 */ "divide by zero",
/* JBIC_CRC_ERROR          9 */ "CRC mismatch",
/* JBIC_INTERNAL_ERROR    10 */ "internal error",
/* JBIC_BOUNDS_ERROR      11 */ "bounds error",
/* JAMC_TYPE_MISMATCH     12 */ "type mismatch",
/* JAMC_ASSIGN_TO_CONST   13 */ "assignment to constant",
/* JAMC_NEXT_UNEXPECTED   14 */ "NEXT unexpected",
/* JAMC_POP_UNEXPECTED    15 */ "POP unexpected",
/* JAMC_RETURN_UNEXPECTED 16 */ "RETURN unexpected",
/* JAMC_ILLEGAL_SYMBOL    17 */ "illegal symbol name",
/* JBIC_VECTOR_MAP_FAILED 18 */ "vector signal name not found",
/* JBIC_USER_ABORT        19 */ "execution cancelled",
/* JBIC_STACK_OVERFLOW    20 */ "stack overflow",
/* JBIC_ILLEGAL_OPCODE    21 */ "illegal instruction code",
/* JAMC_PHASE_ERROR       22 */ "phase error",
/* JAMC_SCOPE_ERROR       23 */ "scope error",
/* JBIC_ACTION_NOT_FOUND  24 */ "action not found",
};

#define MAX_ERROR_CODE (int)((sizeof(error_text)/sizeof(error_text[0]))+1)

static int jtag_io_test(int gpioNum, int action)
{
	
	int pin=0;
	int retval = 0, Value = 0;
	
	switch(gpioNum){
		case TDI_PIN: 	pin = 0; break;
		case TDO_PIN:	pin = 1; break;
		case TCK_PIN:	pin = 2; break;
		case TMS_PIN:	pin = 3; break;
		default:
			dbgprintf("error Pin!!!\n");
			return -1;
			break;
	}

	switch ( action ){
		case SET_GPIO_LOW:
			
			if ( -1 ==  set_jtag_io(pin, action)){
				dbgprintf("Set low failed\n");
				retval = -1;
				goto error_out;
			}
			break;
		case SET_GPIO_HIGH:
			if ( -1 == set_jtag_io(pin, action)){
				dbgprintf("Set Gpio high failed\n");
				retval = -1;
				goto error_out;
			}
			break;

		case READ_GPIO_VAL:
			Value = read_jtag_io(pin, action);
			if ( -1 == Value ){
				dbgprintf("Read Jtag failed\n");
				retval = -1;
	            goto error_out;
			}
			retval = Value;
			break;
			
		case SET_GPIO_DIR_IN:
		case SET_GPIO_DIR_OUT:

			break;	
		}
	error_out:
	    return retval;
}

static int Pin_ctrl(int gpioNum, int action)
{

    int retval = 0, Value = 0;
	switch ( action ) {
        case SET_GPIO_LOW:		
        case SET_GPIO_HIGH:
            jtag_io_test(gpioNum, action);		
            break;
        case READ_GPIO_VAL:
            
            Value = jtag_io_test(gpioNum, action);		
            if ( -1 == Value )
            {
                //printf ( "Read Gpio failed\n");
                retval = -1;
                goto error_out;
            }
            retval = Value;
            break;
            
        case SET_GPIO_DIR_IN:
            
            break;
        case SET_GPIO_DIR_OUT:
            
            break;	
	}

error_out:
    return retval;
}

int jbcmain(char* jbc_action,unsigned long *buf, unsigned long data_size,unsigned char IsBackground)
{
	long error_address = 0L;
	JBI_RETURN_TYPE crc_result = JBIC_SUCCESS;
	JBI_RETURN_TYPE exec_result = JBIC_SUCCESS;
	unsigned short expected_crc = 0;
	unsigned short actual_crc = 0;
	int exit_status = 0;
	int exit_code = 0;
	int format_version = 0;
	char *workspace = NULL;
	char *action = NULL;
	long workspace_size = 0;
	char *exit_string = NULL;
	int reset_jtag = 1;
	int execute_program = 1;
  	char optional[]={OPTIONALT_FUN};
  	char *init_list[10];

  	memset(init_list,0,sizeof(init_list));
  
	/* print out the version string and copyright message */
	dbgprintf("Jam STAPL ByteCode Player Version 2.2\nCopyright (C) 1998-2001 Altera Corporation\n\n");

	action = jbc_action;
	
	// get memory buffer address
	file_buffer = (unsigned char *)buf;
	// get memory buffer size
	file_length = data_size;

  	if(IsBackground!=0)
  	{	
    	init_list[0] = &optional[0];
    	dbgprintf("%s\n",init_list[0]);
  	}

	if ( (workspace_size > 0) && ((workspace = (char *) jbi_malloc((size_t) workspace_size)) == NULL) )
	{
		dbgprintf( "Error: can't allocate memory (%d Kbytes)\n",(int) (workspace_size / 1024L));
		exit_status = 1;
	}
	else
	{
        do{ //for exit(1) changed code.
		if (exit_status == 0)
		{

			/*
			*	Calibrate the delay loop function
			*/
			calibrate_delay();

			/*
			*	Check CRC
			*/
			crc_result = jbi_check_crc(file_buffer, file_length,
				&expected_crc, &actual_crc);

			if (crc_result == JBIC_CRC_ERROR)
			{
				switch (crc_result)
				{
				case JBIC_SUCCESS:
					dbgprintf("CRC matched: CRC value = %04X\n", actual_crc);
					break;

				case JBIC_CRC_ERROR:
					dbgprintf("CRC mismatch: expected %04X, actual %04X\n",
						expected_crc, actual_crc);
					break;

				case JBIC_UNEXPECTED_END:
					dbgprintf("Expected CRC not found, actual CRC value = %04X\n",
						actual_crc);
					break;

				case JBIC_IO_ERROR:
					dbgprintf("Error: File format is not recognized.\n");
					//exit(1);
                    			exec_result = JBIC_IO_ERROR;
					exit_status = 1;
                    			continue;
					break;

				default:
					dbgprintf("CRC function returned error code %d\n", crc_result);
					break;
				}
			}

			if (execute_program)
			{
				/*
				*	Execute the Jam STAPL ByteCode program
				*/
				exec_result = jbi_execute(file_buffer, file_length, workspace,
					workspace_size, action,init_list, reset_jtag,
					&error_address, &exit_code, &format_version);
				
				if (exec_result == JBIC_SUCCESS)
				{
					if (format_version == 2)
					{
						switch (exit_code)
						{
						case  0: exit_string = "Success"; break;
						case  1: exit_string = "Checking chain failure"; break;
						case  2: exit_string = "Reading IDCODE failure"; break;
						case  3: exit_string = "Reading USERCODE failure"; break;
						case  4: exit_string = "Reading UESCODE failure"; break;
						case  5: exit_string = "Entering ISP failure"; break;
						case  6: exit_string = "Unrecognized device"; break;
						case  7: exit_string = "Device revision is not supported"; break;
						case  8: exit_string = "Erase failure"; break;
						case  9: exit_string = "Device is not blank"; break;
						case 10: exit_string = "Device programming failure"; break;
						case 11: exit_string = "Device verify failure"; break;
						case 12: exit_string = "Read failure"; break;
						case 13: exit_string = "Calculating checksum failure"; break;
						case 14: exit_string = "Setting security bit failure"; break;
						case 15: exit_string = "Querying security bit failure"; break;
						case 16: exit_string = "Exiting ISP failure"; break;
						case 17: exit_string = "Performing system test failure"; break;
						default: exit_string = "Unknown exit code"; break;
						}
					}
					else
					{
						switch (exit_code)
						{
						case 0: exit_string = "Success"; break;
						case 1: exit_string = "Illegal initialization values"; break;
						case 2: exit_string = "Unrecognized device"; break;
						case 3: exit_string = "Device revision is not supported"; break;
						case 4: exit_string = "Device programming failure"; break;
						case 5: exit_string = "Device is not blank"; break;
						case 6: exit_string = "Device verify failure"; break;
						case 7: exit_string = "SRAM configuration failure"; break;
						default: exit_string = "Unknown exit code"; break;
						}
					}

                    if((fg_Data_Readback == 0) && (exit_code != 0))
                        dbgprintf("Exit code = %d... %s\n", exit_code, exit_string);
				}
				else if ((format_version == 2) &&
					(exec_result == JBIC_ACTION_NOT_FOUND))
				{
					if ((action == NULL) || (*action == '\0'))
					{
						dbgprintf("Error: no action specified for Jam STAPL file.\nProgram terminated.\n");
					}
					else
					{
						dbgprintf("Error: action \"%s\" is not supported for this Jam STAPL file.\nProgram terminated.\n", action);
					}
				}
				else if (exec_result < MAX_ERROR_CODE)
				{
					dbgprintf("Error at address %ld: %s.\nProgram terminated.\n",
						error_address, error_text[exec_result]); /* SCA Fix [Out-of-bounds read]:: False Positive */
						/* Reason for False Positive - Already check whether the maximum number has been exceeded */
				}
				else
				{
					dbgprintf("Unknown error code %ld\n", (long int)exec_result);
				}

			}
		}
        }while(0);  //for exit(1) changed code.
	}

	if (jtag_hardware_initialized) close_jtag_hardware();

	if (workspace != NULL) jbi_free(workspace);

    if (exec_result != JBIC_SUCCESS) return -1;
    return exit_code;
}

void initialize_jtag_hardware()
{
    //For gpio pin.
	Pin_ctrl(TDI_PIN, SET_GPIO_DIR_OUT);
	Pin_ctrl(TDO_PIN, SET_GPIO_DIR_IN);
	Pin_ctrl(TCK_PIN, SET_GPIO_DIR_OUT);
	Pin_ctrl(TMS_PIN, SET_GPIO_DIR_OUT);
}

void close_jtag_hardware()
{
	fg_Data_Readback = 0;
}

void delay_loop(long count)
{
	while (count != 0L){
		if (count > 20000){//20ms
			msleep(count/1000);			
			count= count % 1000;
		}
		else{
			static unsigned int idle_count = 0;
			//msleep :: for kernel switch other task.
			if((++ idle_count  ) % 1024 == 0){
				msleep(0);
				idle_count = 0;
			}
			udelay( count);
			count-= count;
		}
	}
}
