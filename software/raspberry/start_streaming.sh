##CONFIGURATION:
CAMERA_ID=1 #camera id
WAIT_TIME=0.1s #sampling period
SERVER_HOST=192.168.0.33 #server ip
SERVER_PORT=8080 #server port
JPTRSMORPH_BIN=../JPEG_transmorph_code/jptrsmorph #path to the jptrsmorph binary

BUILD_DIR=build
mkdir -p $BUILD_DIR
VISIBLE_OUTPUT=$BUILD_DIR/visible.jpeg
IR_OUTPUT=$BUILD_DIR/thermal.jpeg

while :
do
  python3 module1_create_visible_jpeg.py $VISIBLE_OUTPUT --simulation;
  python3 module2_create_ir_jpeg.py $IR_OUTPUT --simulation;
  current_time=$(date "+%Y.%m.%d-%H.%M.%S");
  file_name=$BUILD_DIR/$CAMERA_ID-$current_time.jpeg;
  $JPTRSMORPH_BIN -morph $VISIBLE_OUTPUT $IR_OUTPUT $file_name;
  # cp build/thermal.jpeg $file_name;
  python3 module4_send_image_to_server.py $SERVER_HOST $SERVER_PORT $file_name;
  rm build/*.jpeg;
  sleep $WAIT_TIME;
done