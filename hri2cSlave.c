#include "hri2c.h"
#include "hrreport.h"
#include "hros.h"
#include "hrpio.h"
#include "hi_reg_defs.h"
#include "dIIC.h"
#define HR_I2C_SLAVE_DEBUG 
#ifdef HR_I2C_SLAVE_DEBUG
#define HR_i2cSlave_Printf(x) printf(x)
#else
#define HR_i2cSlave_Printf(x)
#endif
/*   全局变量定义     */


typedef enum
{
	OTHERS=0X00,
	DAT_INT_LOW_PULSE=0x01,
	DAT_INT_UP_PULSE=0x02,
	CLK_INT_LOW_PULSE=0x03,
	CLK_INT_UP_PULSE=0x04,
}interupt_source;
typedef enum
{
	SLEEP=0X00,
	WEAK=0x01,
	READ=0x02,
	WRITE=0x03,
	ACK=0x04,
	ACK_END=0X05,
}i2cSlave_status;
typedef enum
{
	NOT_CALL_ME=0,
	CALL_ME_READ=1,
	CALL_ME_WRITE=2
}i2cSlave_call_who;

static D_U32    i2cSlave_sdaHandle,i2cSlave_sclHandle;
static D_U32 slave_device_addr=0xA8;
//static D_U32 test_memo[]={0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1f};
static i2cSlave_status status;
static i2cSlave_call_who call_status;
static D_U8 byte_r=0,bit_r=0,read_buff[32],read_buff_point1=0,read_buff_point2=0,same_circle_r=1;
static D_U8 byte_w=0,bit_w=0,write_buff[32],write_buff_point1=0,write_buff_point2=0,same_circle_w=1;;
static D_U8 ack=1;//reg_addr,reg_data;

extern U8 interrupt_install(U8 Number, U8 Level, U8 TriggerMode, void (*InterruptHandler)(void));
extern U8 interrupt_enable_number(U8 Number);
extern D_U32 pio_powof2(D_U8 number);


/*   IIC从机读1字节     */
D_U8 i2cSlave_read_byte( D_U8  *value)
{
	D_U8 ero=I2C_NO_ERROR;
	if(same_circle_r)
	{
		if (read_buff_point1>read_buff_point2)
		{	
			//有数据可读
			*value= read_buff[read_buff_point2];
			read_buff_point2++;
		}
		else
		{//无数据可读
			read_buff_point2=read_buff_point1;
			ero= I2C_ERROR;
		}
	}
	else
	{
		if(read_buff_point2>read_buff_point1)
		{//有数据可读
			*value= read_buff[read_buff_point2];
			read_buff_point2++;
			if(read_buff_point2==32)
			{	
				read_buff_point2=0;
				same_circle_r=1;
			}
		}
		else//存储数据已超32字节，最老的数据已被刷新
		{
			read_buff_point2=read_buff_point1;
			*value= read_buff[read_buff_point2];
			read_buff_point2++;
			if(read_buff_point2==32)
			{	
				read_buff_point2=0;
				same_circle_r=1;
			}
		}
		
	}
	return ero;
}
/*   IIC从机写1字节     */
D_U8 i2cSlave_write_byte(D_U8 value)
{
	D_U8 ero=I2C_NO_ERROR;
	if(same_circle_w)
	{
		if(write_buff_point2>write_buff_point1)
		{
			write_buff[write_buff_point2]=value;
			write_buff_point2++;
		}
		else
		{//无数据可发
			write_buff_point1=write_buff_point2;
			ero=I2C_ERROR;
		}
	}
	else
	{
		if(write_buff_point1>write_buff_point2)
		{
			write_buff[write_buff_point2]=value;
			write_buff_point2++;
		}
		else
		{
			write_buff_point2=write_buff_point1;
			ero=I2C_ERROR;
		}
	}
	return ero;
}


//判断当前中断源
static interupt_source interupt_from(D_U32 sdaHandle,D_U32 sclHandle)
{
	D_U32 bitmask = 1;
	D_U8 i;
	PIO_ControlParams_t *dat_Pio,*clk_Pio;
	dat_Pio = (PIO_ControlParams_t *)sdaHandle;
	clk_Pio = (PIO_ControlParams_t *)sclHandle;
	
	for(i = 0; i < 32; i++)
	{
		if((HI_GPIO_INT_STATUS) & (bitmask<< i)) //HI_GPIO_INT_STATUS
			break;
	}
	bitmask= pio_powof2(i);
	HI_GPIO_EOI |= bitmask; //HI_GPIO_EOI清中断
	
	if(i == dat_Pio->PortBit)//sdaHandle
		return (HI_GPIO_INT_POLARITY&bitmask)? DAT_INT_UP_PULSE: DAT_INT_LOW_PULSE;
	else if(i==clk_Pio->PortBit)//sclHandle
		return (HI_GPIO_INT_POLARITY&bitmask)? CLK_INT_UP_PULSE: CLK_INT_LOW_PULSE;
	else
		return OTHERS;
}
//中断处理函数
static void i2cSlave_interupt(void)
{
	D_U8 tem;
	interupt_source int_src;
	int_src=interupt_from(i2cSlave_sdaHandle,i2cSlave_sclHandle);
	
	if((int_src==DAT_INT_LOW_PULSE) & hrpio_get(i2cSlave_sclHandle))
	{//START  ----1
	//准备进入READ状态接收device_addr
	//WEAK中继续开双下降沿触发，以防weak中来stop信号
		status=WEAK;
	}
	else if((int_src==DAT_INT_UP_PULSE) & hrpio_get(i2cSlave_sclHandle))
	{//STOP   ----4
	//进入SLEEP状态，开双下降沿中断
		status=SLEEP;
		call_status=NOT_CALL_ME;
		hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
		hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
	}
	else if(int_src==CLK_INT_LOW_PULSE)
	{
		if (status==WEAK)
		{//DAT可开始变换电平，发送端set一位数据 ,接收端准备收一位数据  ----r2
			status=READ;
			hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INPUT);
			hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_UP_PULSE);
		}
		else if(status==WRITE)
		{	
			bit_w++;
			if(bit_w==8)
			{//   ----w8
				bit_w=0;
				status=ACK;
				hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_UP_PULSE);
				hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INPUT);
			}
			else
			{//    ----w2
				if(byte_w&0x80)
					hrpio_set(i2cSlave_sdaHandle);
				else
					hrpio_clear(i2cSlave_sdaHandle);
				byte_w<<=1;
			}
		}
		else if(status==ACK)
		{//DAT 设置ACK位    ----r9
			hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_OUTPUT);
			hrpio_clear(i2cSlave_sdaHandle);//响应主机，回复ACK================还要加判断
			status=ACK_END;
		}
		else if (status==ACK_END)//read/write都进   -----10
		{//还可能出现重复开始信号Sr
			if(call_status==CALL_ME_WRITE)
			{//根据ACK判断是否继续写数据

				if(ack==0)
				{// 收到主机应答，准备继续发送下一字节-----10->w2
					status=WRITE;
					hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_OUTPUT);
					hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
					byte_w=write_buff[write_buff_point1];//取出要发送的第一字节
					write_buff_point1++;
					if (write_buff_point1==32)write_buff_point1=0;
					bit_w=0;
					if(byte_w&0x80)//准备好第一位
						hrpio_set(i2cSlave_sdaHandle);
					else
						hrpio_clear(i2cSlave_sdaHandle);
					byte_w<<=1;
				}
				else//未收到主机应答，停止发数据，准备接收主机停止信号
				{
					status=READ;
					hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INPUT);
					hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_UP_PULSE);
				}
			}
			else if(call_status==CALL_ME_READ)
			{
				status=READ;
				hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INPUT);
				hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_UP_PULSE);
			}
			else
			{
				status=SLEEP;
				hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
				hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
			}
		}
		else if(status==SLEEP)
		{
		hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
		hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
		}
	}
	else if(int_src==CLK_INT_UP_PULSE)
	{
		if(status==READ)
		{//DAT保持电平稳定 ,接收端get一位数据     ----r3
			tem=hrpio_get(i2cSlave_sdaHandle);
			byte_r<<=1;
			byte_r|=tem;
			//byte_r|=tem<<bit_r;
			bit_r++;
			if(tem)//读数据状态时，CLK高电平时，DAT确保能接收start和stop信号
				hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
			else
				hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_UP_PULSE);
		}
		else if(status==ACK)
		{//        -----w9
			ack=hrpio_get(i2cSlave_sdaHandle);
			hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
			status=ACK_END;
		}
	}
	//是否操作完1BYTE 数据,若收完8位数据，位数清零，数据取走后清零
	//接收完8位,准备发送ACK位
	//在下一个CLK下降沿到来时，DAT开始set ACK位
	if(bit_r==8)
	{
		bit_r=0;
		status=ACK;//   ----8
		hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
		if((call_status==NOT_CALL_ME)&((byte_r&0xfe)==slave_device_addr))
		{//解析地址命令
			if(byte_r&0x01)//主机要读数据，要求从机写数据
				call_status=CALL_ME_WRITE;
			else//主机要写数据，要求从机读数据
				call_status=CALL_ME_READ;
			ack=0;
		}
		else if((call_status==NOT_CALL_ME)&(byte_r>>1!=slave_device_addr))
		{//叫别人，没叫我，进入SLEEP状态，等下一个start
			status=SLEEP;
			call_status=NOT_CALL_ME;
			hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
			hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
		}
		else
		{//接收数据,device_addr字节不保存
			read_buff[read_buff_point1]=byte_r;
			read_buff_point1++;
			if(read_buff_point1==32)read_buff_point1=0;//接收数据缓存区满，指针回到0，循环buffer只保留32bytes数据
		}
		byte_r=0;
		
		
	}
	if((status==SLEEP)&(call_status=NOT_CALL_ME))
	{
		hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
		hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
	}

}



//打开中断
static D_Result i2cSlave_config_interupt(void)
{
    D_U8 ret;
    hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
    hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
    ret = interrupt_install(INT_GPIO, 0, 0, i2cSlave_interupt); //INT_GPIO
    ret = interrupt_enable_number(INT_GPIO);
    return ret;
}


/*   IIC从机初始化     */
D_U8 hri2cSlave_init(D_IICOpenParam *param1) //上层传的是I2C用到的PIO端口，及几路I2C的个数
{
	D_U8 ret=0;

	//param1.sda=D_GPIO_PORT3_BIT4;
	//param1.sdc=D_GPIO_PORT3_BIT2;
	ret+=hrpio_open(param1->sda, PIO_BIT_INT_LOW_PULSE, &i2cSlave_sdaHandle);//i2cSlave_sdaHandle
	ret+=hrpio_open(param1->sdc, PIO_BIT_INT_LOW_PULSE, &i2cSlave_sclHandle);//i2cSlave_sclHandle
if(0){	
	hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_OUTPUT);
	hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_OUTPUT);
	hrpio_set(i2cSlave_sdaHandle);
	hrpio_clear(i2cSlave_sdaHandle);
	hrpio_set(i2cSlave_sdaHandle);
	hrpio_clear(i2cSlave_sdaHandle);
	hrpio_set(i2cSlave_sdaHandle);
	hrpio_clear(i2cSlave_sdaHandle);

	hrpio_set(i2cSlave_sclHandle);
	hrpio_clear(i2cSlave_sclHandle);
	hrpio_set(i2cSlave_sclHandle);
	hrpio_clear(i2cSlave_sclHandle);
	hrpio_set(i2cSlave_sclHandle);
	hrpio_clear(i2cSlave_sclHandle);
 	hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
    hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
}
	ret+=i2cSlave_config_interupt();
	if(ret)
		HR_i2cSlave_Printf("@-@ i2cSlave config interupt fail!!");
	else
		HR_i2cSlave_Printf("@-@ i2cSlave init success!!");
	return ret;
}


/*   IIC从机test     */
void i2cSlave_test()
{
	D_U8 addrs[16],value[16]={0x1b,0x7b},read_value[16];
	D_IICOpenParam param_i2cSlave1;
	D_U8 i=1;
	param_i2cSlave1.sda=D_GPIO_PORT3_BIT4;
	param_i2cSlave1.sdc=D_GPIO_PORT3_BIT2;
	addrs[0]=0x05;
	hri2cSlave_init(&param_i2cSlave1);

	while(i){
	HRI2c_Write(0,0xa8, addrs, value,2,1);
	i2cSlave_read_byte(read_value);
	i++;	
		}
}
