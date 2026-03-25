# Data Acquisition Firmware

The data acquisition firmware capture data from the movement modulino and sends
it to the serial port. A corresponding python program then capture such data in
order to save it to a csv file. 

## Commands

### Build
`pio run`

### Flash to board
`pio run --target upload`

### Serial monitor 
`pio device monitor --baud 9600`

### Flash & Monitor 
`pio run --target upload && pio device monitor`

### Clean build
`pio run --target clean`

### Check available serial ports
`pio device list`

### Generate `compile_commands.json`
`pio run --target compiledb`
