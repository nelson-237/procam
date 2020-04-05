CAMERA_ID=1 #modify to camera id
WAIT_TIME=0.1s
SERVER_HOST=192.168.0.33 #modify to server ip
SERVER_PORT=8080
mkdir -p build
while :
do
  python3 module1_create_visible_jpeg.py build/visible.jpeg --simulation;
  python3 module2_create_ir_jpeg.py build/thermal.jpeg --simulation;
  current_time=$(date "+%Y.%m.%d-%H.%M.%S");
  file_name=$CAMERA_ID-$current_time.jpeg;
#   ./jptrsmorph -morph build/visible.jpeg build/thermal.jpeg build/$file_name.jpeg;
  cp build/thermal.jpeg build/$file_name;
  python3 module4_send_image_to_server.py $SERVER_HOST $SERVER_PORT build/$file_name;
  rm build/*.jpeg;
  sleep $WAIT_TIME;
done