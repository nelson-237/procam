# ProCam raspberry software

## Module:
- Module 1 reads a visible camera and produces a jpeg file
- Module 2 reads a IR camera and produces a jpeg file
- Module 3 takes two images and transmorph them to have a combined jpeg file
- Module 4 sends an image to the server

## Usage:
### Module 1:
`python module1_create_visible_jpeg.py <output_image_path>`
### Module 4:
`python module2_create_ir_jpeg.py <output_image_path>`
### Module 3:
`./jptrsmorph -morph <mask_file> -key 8966 rgb_o.jpg rgb_sub.jpg thermal_o.jpg thermal_emb.jpg`
### Module 4:
`python module4_send_image_to_server.py <server_hostname> <server_port> <image_path>`
