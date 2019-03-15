/**
 * @file mcp23s17.c
 * @desc Driver for Microchip MCP23S17 IO Expeander
 * @author Samuel Dewan
 * @date 2019-03-14
 * Last Author:
 * Last Edited On:
 */

#include "mcp23s17.h"
#include "mcp23s17-registers.h"


void init_mcp23s17(struct mcp23s17_desc_t *descriptor, uint8_t address,
                   struct sercom_spi_desc_t *spi_inst, uint32_t cs_pin_mask,
                   uint8_t cs_pin_group)
{
    /* Compute device address */
    descriptor->opcode = MCP23S17_ADDR | (address & 0x7);
    descriptor->spi_out_buffer[0] = descriptor->opcode;
    
    /* Mark all interrupt pins as invalid (interrupt unused) */
    for (uint8_t i = 0; i < MCP23S17_MAX_NUM_INTERRUPTS; i++) {
        descriptor->interrupt_pins[i].invalid = 1;
    }
    
    /* Store SPI settings */
    descriptor->spi_inst = spi_inst;
    descriptor->cs_pin_mask = cs_pin_mask;
    descriptor->cs_pin_group = cs_pin_group;
    
    /* Clear transaction state */
    descriptor->transaction_state = MCP23S17_SPI_NONE;
    
    /* Initilize register cache */
    // Start all pins as inputs
    descriptor->registers.IODIR[0].reg = 0xff;
    descriptor->registers.IODIR[1].reg = 0xff;
    // Ensure that none of the polarities are inverted
    descriptor->registers.IPOL[0].reg = 0x0;
    descriptor->registers.IPOL[1].reg = 0x0;
    // Ensure that all interrupts are disabled
    descriptor->registers.GPINTEN[0].reg = 0x0;
    descriptor->registers.GPINTEN[1].reg = 0x0;
    // Configuration register: hardware address enabled, interrupt pins mirrored
    descriptor->registers.IOCON.reg = (MCP23S17_IOCON_HAEN_Msk |
                                       MCP23S17_IOCON_MIRROR_Msk);
    descriptor->registers.IOCON_ALT = descriptor->registers.IOCON.reg;
    // Ensure that all pull-ups are disabled
    descriptor->registers.GPPU[0].reg = 0x0;
    descriptor->registers.GPPU[1].reg = 0x0;
    // Ensure that all pins are set low
    descriptor->registers.OLAT[0].reg = 0x0;
    descriptor->registers.OLAT[1].reg = 0x0;
    
    /* Set flags to prompt updates to all registers */
    descriptor->gpio_dirty = 1;
    descriptor->config_dirty = 1;
    descriptor->olat_dirty = 1;
    
    /* Start updating registers immediately */
    mcp23s17_service(descriptor);
}

void mcp23s17_service(struct mcp23s17_desc_t *inst)
{
    if ((inst->transaction_state != MCP23S17_SPI_NONE) &&
        sercom_spi_transaction_done(inst->spi_inst, inst->spi_transaction_id)) {
        /* The current SPI transaction has finished */
        
        if (inst->transaction_state == MCP23S17_SPI_INTERRUPTS) {
            /* An interrupts fetch transaction has finished, parse interupt */
            for (uint8_t i = 0; i < MCP23S17_MAX_NUM_INTERRUPTS; i++) {
                union mcp23s17_pin_t pin = inst->interrupt_pins[i];
                if ((!pin.invalid) &&
                    (inst->registers.INTF[pin.port].reg & (1 << pin.pin))) {
                    inst->interrupt_callbacks[i](inst, pin,
                     !!(inst->registers.INTCAP[pin.port].reg & (1 << pin.pin)));
                }
            }
        }
        
        // Clear the transaction state
        inst->transaction_state = MCP23S17_SPI_NONE;
        // Clear the SPI transaction
        sercom_spi_clear_transaction(inst->spi_inst, inst->spi_transaction_id);
    }
    
    
    if (inst->transaction_state == MCP23S17_SPI_NONE) {
        /* No SPI transaction is in progress, start one if needed */
        if (inst->interrupts_dirty) {
            /* Start a transaction to fetch the interrupt registers */
            inst->opcode |= 1;  // Set R/W to read
            inst->reg_addr = MCP23S17_INTFA;
            uint8_t s = sercom_spi_start(inst->spi_inst,
                                         &inst->spi_transaction_id,
                                         MCP23S17_BAUD_RATE, inst->cs_pin_group,
                                         inst->cs_pin_mask, &inst->opcode, 2,
                                         (uint8_t*)&inst->registers.INTF[0], 4);
            if (!s) {
                // Transaction was queued, update state
                inst->transaction_state = MCP23S17_SPI_INTERRUPTS;
                inst->interrupts_dirty = 0;
            }
        } else if (inst->gpio_dirty) {
            /* Start a transaction to fetch the GPIO registers */
            inst->opcode |= 1;  // Set R/W to read
            inst->reg_addr = MCP23S17_GPIOA;
            uint8_t s = sercom_spi_start(inst->spi_inst,
                                         &inst->spi_transaction_id,
                                         MCP23S17_BAUD_RATE, inst->cs_pin_group,
                                         inst->cs_pin_mask, &inst->opcode, 2,
                                         (uint8_t*)&inst->registers.GPIO[0], 2);
            if (!s) {
                // Transaction was queued, update state
                inst->transaction_state = MCP23S17_SPI_GPIO;
                inst->gpio_dirty = 0;
            }
        } else if (inst->config_dirty) {
            /* Start a transaction to update the configuration registers */
            inst->opcode &= ~1;  // Clear R/W to write
            inst->reg_addr = MCP23S17_IODIRA;
            uint8_t s = sercom_spi_start(inst->spi_inst,
                                         &inst->spi_transaction_id,
                                         MCP23S17_BAUD_RATE, inst->cs_pin_group,
                                         inst->cs_pin_mask, &inst->opcode, 20,
                                         NULL, 0);
            if (!s) {
                // Transaction was queued, update state
                inst->transaction_state = MCP23S17_SPI_OTHER;
                inst->config_dirty = 0;
            }
        } else if (inst->olat_dirty) {
            /* Start a transaction to update the output latch registers */
            inst->spi_out_buffer[0] &= ~1;  // Clear R/W to write
            inst->spi_out_buffer[1] = MCP23S17_OLATA;
            inst->spi_out_buffer[2] = inst->registers.OLAT[0].reg;
            inst->spi_out_buffer[3] = inst->registers.OLAT[1].reg;
            uint8_t s = sercom_spi_start(inst->spi_inst,
                                         &inst->spi_transaction_id,
                                         MCP23S17_BAUD_RATE, inst->cs_pin_group,
                                         inst->cs_pin_mask,
                                         &inst->spi_out_buffer, 4, NULL, 0);
            if (!s) {
                // Transaction was queued, update state
                inst->transaction_state = MCP23S17_SPI_OTHER;
                inst->olat_dirty = 0;
            }
        }
    }
}

void mcp23s17_set_pin_mode(struct mcp23s17_desc_t *inst,
                           union mcp23s17_pin_t pin, uint8_t mode)
{
    if ((mode == MCP23S17_MODE_OUTPUT) &&
        (inst->registers.IODIR[pin.port].reg & (1 << pin.pin))) {
        // Set pin as output
        inst->registers.IODIR[pin.port].reg &= ~(1 << pin.pin);
        // Mark configuration to be updated
        inst->config_dirty = 1;
    } else if (!(inst->registers.IODIR[pin.port].reg & (1 << pin.pin))) {
        // Set pin as input
        inst->registers.IODIR[pin.port].reg |= (1 << pin.pin);
        // Mark configuration to be updated
        inst->config_dirty = 1;
    }
    // Start the update immediately if possible
    mcp23s17_service(inst);
}

void mcp23s17_set_output(struct mcp23s17_desc_t *inst,
                         union mcp23s17_pin_t pin, uint8_t value)
{
    if ((value == MCP23S17_VALUE_LOW) &&
        (inst->registers.OLAT[pin.port].reg & (1 << pin.pin))) {
        // Set pin low
        inst->registers.OLAT[pin.port].reg &= ~(1 << pin.pin);
        // Mark output latch to be updated
        inst->olat_dirty = 1;
    } else if (!(inst->registers.OLAT[pin.port].reg & (1 << pin.pin))) {
        // Set pin high
        inst->registers.OLAT[pin.port].reg |= (1 << pin.pin);
        // Mark output latch to be updated
        inst->olat_dirty = 1;
    }
    // Start the update immediately if possible
    mcp23s17_service(inst);
}

void mcp23s17_set_pull_up(struct mcp23s17_desc_t *inst,
                          union mcp23s17_pin_t pin, uint8_t value)
{
    if ((value == MCP23S17_PULL_UP_DISABLED) &&
        (inst->registers.GPPU[pin.port].reg & (1 << pin.pin))) {
        // Disable pull-up
        inst->registers.GPPU[pin.port].reg &= ~(1 << pin.pin);
        // Mark configuration to be updated
        inst->config_dirty = 1;
    } else if (!(inst->registers.GPPU[pin.port].reg & (1 << pin.pin))) {
        // Enable pull-up
        inst->registers.GPPU[pin.port].reg |= (1 << pin.pin);
        // Mark configuration to be updated
        inst->config_dirty = 1;
    }
    // Start the update immediately if possible
    mcp23s17_service(inst);
}

uint8_t mcp23s17_enable_interrupt(struct mcp23s17_desc_t *inst,
                                  union mcp23s17_pin_t pin,
                                  enum mcp23s17_interrupt_type type,
                                  mcp23s17_int_callback callback)
{
    for (int8_t i = 0; i < MCP23S17_MAX_NUM_INTERRUPTS; i++) {
        if (inst->interrupt_pins[i].invalid) {
            // Configure interrupt in instance descriptor
            inst->interrupt_pins[i] = pin;
            inst->interrupt_callbacks[i] = callback;
            
            // Enable interrupt on device
            inst->registers.GPINTEN[pin.port].reg |= (1 << pin.pin);
            switch (type) {
                case MCP23S17_INT_ON_CHANGE:
                    if (inst->registers.INTCON[pin.port].reg & (1 << pin.pin)) {
                        // Set interrupt trigger type to pin change
                        inst->registers.INTCON[pin.port].reg &= ~(1 << pin.pin);
                        // Mark device configuration registers to be updated
                        inst->config_dirty = 1;
                    }
                    break;
                case MCP23S17_INT_FALLING_EDGE:
                    if (!(inst->registers.INTCON[pin.port].reg & (1 << pin.pin))) {
                        // Set interrupt trigger type to edge detect
                        inst->registers.INTCON[pin.port].reg |= (1 << pin.pin);
                        // Mark device configuration registers to be updated
                        inst->config_dirty = 1;
                    }
                    if (!(inst->registers.DEFVAL[pin.port].reg & (1 << pin.pin))) {
                        // Set edge to falling edge
                        inst->registers.DEFVAL[pin.port].reg |= (1 << pin.pin);
                        // Mark device configuration registers to be updated
                        inst->config_dirty = 1;
                    }
                    break;
                case MCP23S17_INT_RISING_EDGE:
                    if (!(inst->registers.INTCON[pin.port].reg & (1 << pin.pin))) {
                        // Set interrupt trigger type to edge detect
                        inst->registers.INTCON[pin.port].reg |= (1 << pin.pin);
                        // Mark device configuration registers to be updated
                        inst->config_dirty = 1;
                    }
                    if (inst->registers.DEFVAL[pin.port].reg & (1 << pin.pin)) {
                        // Set edge to rising edge
                        inst->registers.DEFVAL[pin.port].reg &= ~(1 << pin.pin);
                        // Mark device configuration registers to be updated
                        inst->config_dirty = 1;
                    }
                    
                    break;
            }
            // Start updating registers now if there is not already an operation
            // in progress
            mcp23s17_service(inst);
            return 0;
        }
    }
    
    // Could not find a free interrupt slot
    return 1;
}

void mcp23s17_disable_interrupt(struct mcp23s17_desc_t *inst,
                                union mcp23s17_pin_t pin)
{
    for (int8_t i = 0; i < MCP23S17_MAX_NUM_INTERRUPTS; i++) {
        if (inst->interrupt_pins[i].value == pin.value) {
            // Disable interrupt in instance descriptor
            inst->interrupt_pins[i].invalid = 1;
            if (inst->registers.GPINTEN[pin.port].reg & (1 << pin.pin)) {
                // Disable interrupt on device
                inst->registers.GPINTEN[pin.port].reg &= ~(1 << pin.pin);
                // Mark device configuration registers as needing to be updated
                inst->config_dirty = 1;
                // Start updating registers now if there is not already an
                // operation in progress
                mcp23s17_service(inst);
            }
            break;
        }
    }
}

void mcp23s17_handle_interrupt(struct mcp23s17_desc_t *inst)
{
    // Mark the interrupt registers as needing to be fetched
    inst->interrupts_dirty = 1;
    // Start fetching the registers immediately if possible
    mcp23s17_service(inst);
}