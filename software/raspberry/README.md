# ProCam raspberry software

## Modules:
We break the streaming cycle into 4 modules:
- Module 1 reads a visible camera and produces a jpeg file
- Module 2 reads a IR camera and produces a jpeg file
- Module 3 takes two images and transmorph them to have a combined jpeg file
- Module 4 sends an image to the server

We provide the `start_streaming.sh` script that calls each module in order and loops forever

## Installation:
 - You need to compile the jptrsmorph library (see the library's `README.md`)
 - Instal the python libraries `pip3 install -r requirements.txt`


## Configuration:
In `start_streaming.sh`, modify 
the configuration values:
- `CAMERA_ID` : id of the camera
- `WAIT_TIME` : spleep time between each cycle
- `SERVER_HOST` : ip of the server
- `SERVER_PORT` : port of the server
- `JPTRSMORPH_BIN` : path to the  jptrsmorph binary 

## Usage:
### Start recording:
 - run `./start_streaming.sh`
### Module 1:
- to record from the camera:`python3 module1_create_visible_jpeg.py <output_image_path>`
- to simulate the camera with random images add the flag `--simulation`
### Module 2:
- to record from the camera: `python3 module2_create_ir_jpeg.py <output_image_path>`
- to simulate the camera with random images add the flag `--simulation`
### Module 3:
`./jptrsmorph -morph <input_file_1> <input_file_2> <output_file>`
### Module 4:
`python3 module4_send_image_to_server.py <server_hostname> <server_port> <image_path>`
