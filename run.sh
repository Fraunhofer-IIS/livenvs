
source $(conda info --base)/bin/activate torchgl

export CC=gcc-11
export CXX=g++-11
# Set this to either g++-7 or 9
export CUDAHOSTCXX=g++-9

mkdir build
cd build
export CONDA=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="${CONDA}/lib/python3.9/site-packages/torch/;${CONDA}" ..
make -j && cd ../src && ./livenvs \
    --datasets=../dataset_configs/livevs_config.yml \
    --preload_dataset \
    -e "../data/traces/traced_enc_net_736x1280.pt" \
    -r "../data/traces/traced_ref_unet_736x1280.pt" \
    -o "../data/traces/traced_out_conv_968x1296.pt" \
|| cd ../src
