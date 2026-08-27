make clean && make PAYLOAD=RemoteBoot MACHINE=VisionFive2 

./machine/riscv64/VisionFive2/tools/vf2-imager/vf2-imager -i build/Image -o build/VisionFive2
./machine/riscv64/VisionFive2/tools/vf2/vf2 build/VisionFive2


