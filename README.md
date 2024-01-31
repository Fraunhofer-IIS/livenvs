# LiveNVS: Neural View Synthesis on Live RGB-D Streams
by Laura Fink, Darius Rueckert, Linus Franke, Joachim Keinert, and Marc Stamminger


![teaser](teaser.png)

Official repository   |   [Link to project page](https://lorafib.github.io/livenvs/)   |   [My home page](https://lorafib.github.io/)

Abstract: 
Existing real-time RGB-D reconstruction approaches, like Kinect Fusion, lack real-time photo-realistic visualization. This is due to noisy, oversmoothed or incomplete geometry and blurry textures which are fused from imperfect depth maps and camera poses. Recent neural rendering methods can overcome many of such artifacts but are mostly optimized for offline usage, hindering the integration into a live reconstruction pipeline.

In this paper, we present LiveNVS, a system that allows for neural novel view synthesis on a live RGB-D input stream with very low latency and real-time rendering. Based on the RGB-D input stream, novel views are rendered by projecting neural features into the target view via a densely fused depth map and aggregating the features in image-space to a target feature map. A generalizable neural network then translates the target feature map into a high-quality RGB image. LiveNVS achieves state-of-the-art neural rendering quality of unknown scenes during capturing, allowing users to virtually explore the scene and assess reconstruction quality in real-time. 


Notes: 
For the sake of simplicity, this project only supports novel view synthesis for pre-recorded datasets without the interface to the SLAM module or camera SDKs. COLMAP, scannet, and tum formats are supported for camera poses. Besides, we are currently considering to publish the datasets recorded for this publication. If a full prototype or (early) access to the dataset is of interest for you, drop me a mail (Laura Fink) or open an issue.

## Citation

```
@inproceedings{fink2023livenvs,
  title={LiveNVS: Neural View Synthesis on Live RGB-D Streams},
  author={Fink, Laura and R{\"u}ckert, Darius and Franke, Linus and Keinert, Joachim and Stamminger, Marc},
  booktitle={SIGGRAPH Asia 2023 Conference Papers},
  pages={1--11},
  year={2023}
}
```


## Prerequisites

### System (tested)
* Ubuntu 20.04/22.04
* gcc/++ >= 11 ( & g++-9 for cuda?)
* cuda-11.6 capable system
* Conda installed and initialized
* ~20GB of free space

### Packages
```
sudo apt install -y build-essential libx11-dev xorg-dev libopengl-dev freeglut3-dev cmake
sudo apt install -y libassimp-dev # optional, to reduce compile times
```

## Setup

Configure environment. The script will create a conda environment "torchgl" (including cuda-11.6 and cudnn) and download libtorch.
```
chmod +x ./configure.sh
./configure.sh
```

## Run

In run.sh, check the set compiler versions and adapt to you system if neccessary. 

```
chmod +x ./run.sh
./run.sh
```

## (Optional) VSCode config
See the exemplary ```settings.json``` and ```launch.json``` in ```./example_vscode_files```. Use  ```vsc_env_init.sh``` to start vs code with the torchgl env activated and other environment variables set.



## Usage

* F2: Use free view point camera
    * Camera controls: w,a,s,d,q,e,r,f
    * Scroll wheel for cam speed
* F3: Use dataset camera (Active on launch)
    * Left and right arrow keys to move along trajectory

* F1: GUI
    * "Draw Auxiliaries" shows confidence map and shaded depth map
    * "Show SVG Menu" will show currently used views, the upmost checkbox "Draw Selected Frusta" addiationally draws their tiny frusta in the scene 
    * "Render Mode" to switch between neural and non-neural mode / forward and deferred warping mode
    * Using more "Num Views" than "Cache Size" available will cause undefined behavior

