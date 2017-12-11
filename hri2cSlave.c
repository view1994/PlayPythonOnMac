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
/*   ȫ�ֱ�������     */


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


/*   IIC�ӻ���1�ֽ�     */
D_U8 i2cSlave_read_byte( D_U8  *value)
{
	D_U8 ero=I2C_NO_ERROR;
	if(same_circle_r)
	{
		if (read_buff_point1>read_buff_point2)
		{	
			//�����ݿɶ�
			*value= read_buff[read_buff_point2];
			read_buff_point2++;
		}
		else
		{//�����ݿɶ�
			read_buff_point2=read_buff_point1;
			ero= I2C_ERROR;
		}
	}
	else
	{
		if(read_buff_point2>read_buff_point1)
		{//�����ݿɶ�
			*value= read_buff[read_buff_point2];
			read_buff_point2++;
			if(read_buff_point2==32)
			{	
				read_buff_point2=0;
				same_circle_r=1;
			}
		}
		else//�洢�����ѳ�32�ֽڣ����ϵ������ѱ�ˢ��
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
/*   IIC�ӻ�д1�ֽ�     */
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
		{//�����ݿɷ�
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


//�жϵ�ǰ�ж�Դ
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
	HI_GPIO_EOI |= bitmask; //HI_GPIO_EOI���ж�
	
	if(i == dat_Pio->PortBit)//sdaHandle
		return (HI_GPIO_INT_POLARITY&bitmask)? DAT_INT_UP_PULSE: DAT_INT_LOW_PULSE;
	else if(i==clk_Pio->PortBit)//sclHandle
		return (HI_GPIO_INT_POLARITY&bitmask)? CLK_INT_UP_PULSE: CLK_INT_LOW_PULSE;
	else
		return OTHERS;
}
//�жϴ�����
static void i2cSlave_interupt(void)
{
	D_U8 tem;
	interupt_source int_src;
	int_src=interupt_from(i2cSlave_sdaHandle,i2cSlave_sclHandle);
	
	if((int_src==DAT_INT_LOW_PULSE) & hrpio_get(i2cSlave_sclHandle))
	{//START  ----1
	//׼������READ״̬����device_addr
	//WEAK�м�����˫�½��ش������Է�weak����stop�ź�
		status=WEAK;
	}
	else if((int_src==DAT_INT_UP_PULSE) & hrpio_get(i2cSlave_sclHandle))
	{//STOP   ----4
	//����SLEEP״̬����˫�½����ж�
		status=SLEEP;
		call_status=NOT_CALL_ME;
		hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
		hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
	}
	else if(int_src==CLK_INT_LOW_PULSE)
	{
		if (status==WEAK)
		{//DAT�ɿ�ʼ�任��ƽ�����Ͷ�setһλ���� ,���ն�׼����һλ����  ----r2
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
		{//DAT ����ACKλ    ----r9
			hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_OUTPUT);
			hrpio_clear(i2cSlave_sdaHandle);//��Ӧ�������ظ�ACK================��Ҫ���ж�
			status=ACK_END;
		}
		else if (status==ACK_END)//read/write����   -----10
		{//�����ܳ����ظ���ʼ�ź�Sr
			if(call_status==CALL_ME_WRITE)
			{//����ACK�ж��Ƿ����д����

				if(ack==0)
				{// �յ�����Ӧ��׼������������һ�ֽ�-----10->w2
					status=WRITE;
					hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_OUTPUT);
					hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
					byte_w=write_buff[write_buff_point1];//ȡ��Ҫ���͵ĵ�һ�ֽ�
					write_buff_point1++;
					if (write_buff_point1==32)write_buff_point1=0;
					bit_w=0;
					if(byte_w&0x80)//׼���õ�һλ
						hrpio_set(i2cSlave_sdaHandle);
					else
						hrpio_clear(i2cSlave_sdaHandle);
					byte_w<<=1;
				}
				else//δ�յ�����Ӧ��ֹͣ�����ݣ�׼����������ֹͣ�ź�
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
		{//DAT���ֵ�ƽ�ȶ� ,���ն�getһλ����     ----r3
			tem=hrpio_get(i2cSlave_sdaHandle);
			byte_r<<=1;
			byte_r|=tem;
			//byte_r|=tem<<bit_r;
			bit_r++;
			if(tem)//������״̬ʱ��CLK�ߵ�ƽʱ��DATȷ���ܽ���start��stop�ź�
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
	//�Ƿ������1BYTE ����,������8λ���ݣ�λ�����㣬����ȡ�ߺ�����
	//������8λ,׼������ACKλ
	//����һ��CLK�½��ص���ʱ��DAT��ʼset ACKλ
	if(bit_r==8)
	{
		bit_r=0;
		status=ACK;//   ----8
		hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
		if((call_status==NOT_CALL_ME)&((byte_r&0xfe)==slave_device_addr))
		{//������ַ����
			if(byte_r&0x01)//����Ҫ�����ݣ�Ҫ��ӻ�д����
				call_status=CALL_ME_WRITE;
			else//����Ҫд���ݣ�Ҫ��ӻ�������
				call_status=CALL_ME_READ;
			ack=0;
		}
		else if((call_status==NOT_CALL_ME)&(byte_r>>1!=slave_device_addr))
		{//�б��ˣ�û���ң�����SLEEP״̬������һ��start
			status=SLEEP;
			call_status=NOT_CALL_ME;
			hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
			hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
		}
		else
		{//��������,device_addr�ֽڲ�����
			read_buff[read_buff_point1]=byte_r;
			read_buff_point1++;
			if(read_buff_point1==32)read_buff_point1=0;//�������ݻ���������ָ��ص�0��ѭ��bufferֻ����32bytes����
		}
		byte_r=0;
		
		
	}
	if((status==SLEEP)&(call_status=NOT_CALL_ME))
	{
		hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
		hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
	}

}



//���ж�
static D_Result i2cSlave_config_interupt(void)
{
    D_U8 ret;
    hrpio_setconfig(i2cSlave_sdaHandle,PIO_BIT_INT_LOW_PULSE);
    hrpio_setconfig(i2cSlave_sclHandle,PIO_BIT_INT_LOW_PULSE);
    ret = interrupt_install(INT_GPIO, 0, 0, i2cSlave_interupt); //INT_GPIO
    ret = interrupt_enable_number(INT_GPIO);
    return ret;
}


/*   IIC�ӻ���ʼ��     */
D_U8 hri2cSlave_init(D_IICOpenParam *param1) //�ϲ㴫����I2C�õ���PIO�˿ڣ�����·I2C�ĸ���
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


/*   IIC�ӻ�test     */
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
