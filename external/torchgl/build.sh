source $(conda info --base)/bin/activate torchgl

export CC=gcc-12
export CXX=g++-12
# Set this to either g++-7 or 9
export CUDAHOSTCXX=g++-9

mkdir build
cd build
export CONDA=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}
cmake -DCMAKE_PREFIX_PATH="${CONDA}/lib/python3.9/site-packages/torch/;${CONDA}" ..
make -j