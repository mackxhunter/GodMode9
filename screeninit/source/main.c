#include <cpu.h>
#include <pxi.h>
#include <gic.h>
#include <i2c.h>
#include <gpulcd.h>
#include <vram.h>
#include <types.h>

vu32 *entrypoint = (vu32*)0x1FFFFFFC;

void PXI_IRQHandler(void)
{
    char pxi_buf[PXI_MAXBUFLEN] = {0};
    u32 pxi_args[PXI_FIFO_LEN]  = {0};
    u8 pxi_cmd;

    pxi_cmd = PXI_GetRemote();
    switch (pxi_cmd) {
    default:
        break;

    case PXI_BRIGHTNESS:
    {
        PXI_RecvArray(pxi_args, 1);
        PXI_SetRemote(PXI_BUSY);
        LCD_SetBrightness(0, pxi_args[0]);
        LCD_SetBrightness(1, pxi_args[0]);
        break;
    }

    /* New CMD template:
    case CMD_ID:
    {
        <var declarations/assignments>
        <receive args from PXI FIFO>
        <if necessary, copy stuff to pxi_buf>
        PXI_SetRemote(PXI_BUSY);
        <execute the command>
        break;
    }
    */
    }

    PXI_SetRemote(PXI_READY);
    return;
}

void main(void)
{
    u32 entry;
    PXI_Reset();

    GPU_Init();
    GPU_PSCFill(VRAM_START, VRAM_END, 0);
    GPU_SetFramebuffers((u32[]){VRAM_TOP_LA, VRAM_TOP_LB,
                                VRAM_TOP_RA, VRAM_TOP_RB,
                                VRAM_BOT_A, VRAM_BOT_B});

    GPU_SetFramebufferMode(0, PDC_RGB24);
    GPU_SetFramebufferMode(1, PDC_RGB24);

    I2C_writeReg(I2C_DEV_MCU, 0x22, 0x2A);

    GIC_Reset();
    GIC_SetIRQ(IRQ_PXI_SYNC, PXI_IRQHandler);
    PXI_EnableIRQ();
    CPU_EnableIRQ();

    PXI_SetRemote(PXI_READY);

    *entrypoint = 0;
    while((entry=*entrypoint) == 0);

    CPU_DisableIRQ();
    PXI_DisableIRQ();
    PXI_Reset();
    GIC_Reset();

    ((void (*)())(entry))();
}
