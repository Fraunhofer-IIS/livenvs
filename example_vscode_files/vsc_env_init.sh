# put  "cmake.configureSettings" : { "CMAKE_PREFIX_PATH":"\"/home/user/miniconda3/envs/torchgl/lib/python3.9/site-packages/torch/;/home/user/miniconda3/envs/torchgl\""     }
# into vscode settings.json

source $(conda info --base)/bin/activate torchgl

export CC=gcc-12
export CXX=g++-12
# Set this to either g++-7 or 9
export CUDAHOSTCXX=g++-9

export CONDA=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}

code .