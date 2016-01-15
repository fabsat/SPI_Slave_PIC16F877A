/******************************************************************************************
 * spi_slave.c
 * ver1.00
 * Hirofumi Hamada
 *=========================================================================================
 * PIC16F877A用SPI通信(Slave)ライブラリソースファイル
 *
 *=========================================================================================
 * ・ver1.00 || 初版 || 2015/12/28 || Hirofumi Hamada
 *   初版作成
 *=========================================================================================
 * PIC16F877A
 * MPLAB X IDE(V3.10/Ubuntu)
 * XC8 (V1.35/Linux)
 *=========================================================================================
 * Created by fabsat Project(Tokai university Satellite Project[TSP])
 *****************************************************************************************/

#include <xc.h>
#include "spi_slave.h"
#include "pic_clock.h"
#include "uart.h"

/* Prototype of static function */
static void spi_reset(void);


/*=====================================================
 * @brief
 *     SPI Slave初期化関数
 * @param
 *     spi_isr:spi割り込みの有効無効を設定
 * @return
 *     なし
 * @note
 *     Pin43:SDO(RC5)
 *     Pin42:SDI(RC4/SDA)
 *     Pin37:SCK(RC3/SCL)
 *     Pin24:SS (RA5/AN4/C2OUT)
 *===================================================*/
void spi_slave_init(spi_isr_set_t spi_isr)
{
    /* SPI pin I/O Configuration */
    TRISC3 = 1;               // RC3 = SCK -> INPUT
    TRISC4 = 1;               // RC4 = SDI -> INPUT
    TRISC5 = 0;               // RC5 = SDO -> OUTPUT
    TRISA5 = 1;               // RA5 = SS  -> INPUT
    ADCON0bits.ADON  = 0;     // RA5 digital Setting
    ADCON1bits.PCFG3 = 0;     // Have to configure to use SS control enable
    ADCON1bits.PCFG2 = 1;
    ADCON1bits.PCFG1 = 1;

    /* Allow Programming of SPI configuration */
    SSPCONbits.SSPEN = 0;

    /*  SPI Mode Setup */
    SSPSTATbits.SMP = 0;    // SMP must be cleared when SPI is used in mode
    SSPCONbits.CKP  = 0;    // Idle state for clock is a low level
    SSPSTATbits.CKE = 0;    // Transmit occurs on transition from idle to active clock state
    
    /* SPI Slave mode, clock = SCK pin. SS pin control enabled. */
    SSPCONbits.SSPM3 = 0;
    SSPCONbits.SSPM2 = 1;
    SSPCONbits.SSPM1 = 0;
    SSPCONbits.SSPM0 = 0;

    /* Enable SPI Interrupt */
    if(spi_isr == spi_isr_enable)
    {
        PIR1bits.SSPIF  = 0;
        PIE1bits.SSPIE  = 1;     // SPI Interrupt enable
        INTCONbits.PEIE = 1;
        INTCONbits.GIE  = 1;        
    }

    /* End SPI programming and Start serial port */
    SSPCONbits.SSPEN = 1;    // Enables serial port and configures SCK, SDO, SDI, and SS as serial port pins
}


/*=====================================================
 * @brief
 *     SPI Interrupt function
 * @param
 *     none:
 * @return
 *     none:
 * @note
 *     none
 *===================================================*/
void spi_interrupt(void)
{
    char data;

    data = SSPBUF;

    put_char(data);  
}



/*=====================================================
 * @brief
 *     SPIデータ送受信関数(1Byte)
 * @param
 *     data:送信データ(char型)
 * @return
 *     received_data:受信データwait for finish
 *     -1           :timeout終了時
 * @note
 *     ・通信完了フラグの待ち状態でreset_counterが
 *       0になったらresetをかける
 *     ・2回resetをかけても完了しなければtimeoutと
 *       して-1を返す
 *===================================================*/
char spi_send_receive(char data)
{
    char received_data;
    unsigned char timeout_counter = 0;
    unsigned char reset_counter   = 0xFF;

    /* transmit data into SSPBUF */
    SSPBUF = data;

    /* Waiting for SPI finish */
    while(SSPSTATbits.BF == 0)
    {
        put_string("into while\r\n");
        /* reset counter overflow sequence */
        if(reset_counter == 0x00 && timeout_counter < 2)
        {
            spi_reset();
            timeout_counter++;
            continue;
        }
        
        /* SPI timeout */
        if(timeout_counter == 2)
        {
            return -1;
        }
        
        /* reset counter decrement */
        reset_counter--;
    }

    /* Get received data from register */
    received_data = SSPBUF;
    SSPBUF = 0xFF;

    return received_data;
}


/*-----------------------------------------------------
 * @brief
 *     SPIリセット関数
 * @param
 *     なし
 * @return
 *     なし
 * @note
 *     待ち状態のreset_counterによって呼び出される
 *---------------------------------------------------*/
static void spi_reset(void)
{
    unsigned char dummy;
    
    SSPEN = 0;         //  Reset SPI module
    SSPEN = 1;         //  Reset SPI module
    dummy = SSPBUF;
    SSPIF = 0;
    SSPEN = 0;         //  Reset SPI module
    SSPEN = 1;         //  Reset SPI module
}
