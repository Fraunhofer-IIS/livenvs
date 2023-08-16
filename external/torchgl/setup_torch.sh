
source $(conda info --base)/etc/profile.d/conda.sh

conda update -n base -c defaults conda

conda create -y -n torchgl python=3.9.7
conda activate torchgl

conda install -y ncurses=6.3 -c conda-forge
#conda install -y cudnn=8.2.1.32 cudatoolkit-dev=11.3 cudatoolkit=11.3 -c nvidia -c conda-forge
conda install -y cudnn=8.2.1.32 cudatoolkit-dev=11.6 cudatoolkit=11.6 -c nvidia -c conda-forge
conda install -y astunparse numpy ninja pyyaml mkl mkl-include setuptools=58.0.4 cmake=3.19.6 cffi typing_extensions future six requests dataclasses pybind11=2.6.2
conda install -y magma-cuda110 -c pytorch
conda install -y freeimage=3.17 jpeg=9d protobuf=3.13.0.1 -c conda-forge


if [ -d "external/thirdparty" ] 
then
    echo "Error: Directory external/thirdparty exists."
else
    echo "Error: Directory external/thirdparty does not exist. create.."
    mkdir external/thirdparty
fi
cd "external/thirdparty"

conda activate torchgl

wget https://download.pytorch.org/libtorch/cu116/libtorch-cxx11-abi-shared-with-deps-1.13.1%2Bcu116.zip -O  libtorch.zip
unzip libtorch.zip -d .


cp -rv libtorch/ $CONDA_PREFIX/lib/python3.9/site-packages/torch/
