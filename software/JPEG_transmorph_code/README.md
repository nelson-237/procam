# JPEG transmorph library

## Compilation:
```
cd src
make
cd ..
./compile_transmorph.sh
```

## Updates:
For ProCam we updated `jptrsmorph.c` to simplify the signature,
we left the original code in the comments as follows:
```c
// procam update
//writeFileToMarker( &dstinfo2, argv[file_index + 1], 11, filesize );
writeFileToMarker( &dstinfo2, static_output_filename, 11, filesize );
```

## Usage:
To embed an image in a second one:
`./jptrsmorph -morph <image_to_embed> <image_to_embed_into> <output_file>`

To extract the embeded image:
`./jptrsmorph -remorph <combined_image> <extracted_image>`