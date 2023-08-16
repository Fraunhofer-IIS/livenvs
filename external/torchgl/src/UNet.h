/**
 * Copyright (c) 2020 Laura Fink
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 * Adapted from saiga project by Darius Rueckert.
 */
#pragma once

#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>
#include <torch/torch.h>
#include <vector>

struct GatedConvImpl : torch::nn::Module
{
    GatedConvImpl(int in_channels, int out_channels)
        : conv(torch::nn::Conv2dOptions(in_channels, out_channels, /*kernel_size=*/3)),
          mask_conv(torch::nn::Conv2dOptions(in_channels, out_channels, /*kernel_size=*/3)),
          bn1(torch::nn::BatchNormOptions(out_channels))
    {
        register_module("conv", conv);
        register_module("mask_conv", mask_conv);
        register_module("bn1", bn1);

        conv->options.padding(1);
        mask_conv->options.padding(1);
    }

    torch::Tensor forward(torch::Tensor input)
    {
        auto x    = conv(input);
        auto mask = mask_conv(input);

        x = torch::relu(x) * torch::sigmoid(mask);
        x = bn1->forward(x);
        return x;
    }
    torch::nn::Conv2d conv;
    torch::nn::Conv2d mask_conv;
    torch::nn::BatchNorm2d bn1;

};
TORCH_MODULE(GatedConv);

struct DoubleConvImpl : torch::nn::Module
{
    DoubleConvImpl(int in_channels, int out_channels)
        : conv1(torch::nn::Conv2dOptions(in_channels, out_channels, /*kernel_size=*/3)),
          conv2(torch::nn::Conv2dOptions(out_channels, out_channels, /*kernel_size=*/3)),
          bn1(torch::nn::BatchNormOptions(out_channels)),
          bn2(torch::nn::BatchNormOptions(out_channels))
    {
        register_module("conv1", conv1);
        register_module("conv2", conv2);
        //        register_module("bn1", bn1);
        //        register_module("bn2", bn2);

        conv1->options.padding(1);
        conv2->options.padding(1);
    }


    torch::Tensor forward(torch::Tensor x)
    {
        x = conv1->forward(x);
        //        x = bn1->forward(x);
        x = torch::relu(x);

        x = conv2->forward(x);
        //        x = bn2->forward(x);
        x = torch::relu(x);

        return x;
    }
    torch::nn::Conv2d conv1;
    torch::nn::Conv2d conv2;
    torch::nn::BatchNorm2d bn1, bn2;
};
TORCH_MODULE(DoubleConv);

struct DownImpl : torch::nn::Module
{
    DownImpl(int in_channels, int out_channels)
        : conv1(in_channels, out_channels), conv2(out_channels, out_channels), mp(2)
    {
        register_module("conv1", conv1);
        register_module("conv2", conv2);
        register_module("mp", mp);
    }


    torch::Tensor forward(torch::Tensor x)
    {
        x = mp->forward(x);
        x = conv1(x);
        x = conv2(x);
        //        x = dc->forward(x);

        return x;
    }
    //    torch::nn::MaxPool2d mp;
    //    DoubleConv dc;
    GatedConv conv1, conv2;
    torch::nn::AvgPool2d mp;
};

TORCH_MODULE(Down);

struct UpImpl : torch::nn::Module
{
    UpImpl(int in_channels, int out_channels) : conv1(in_channels, out_channels), conv2(out_channels, out_channels)
    {
        register_module("conv1", conv1);
        register_module("conv2", conv2);
    }


    torch::Tensor forward(torch::Tensor x1, torch::Tensor x2)
    {
        //        std::cout << x1.sizes() << " " << x2.sizes() << std::endl;
        x1 = torch::upsample_bilinear2d(x1, {2 * x1.size(2), 2 * x1.size(3)}, false);

        int diffY = x2.size(2) - x1.size(2);
        int diffX = x2.size(3) - x1.size(3);
        x1        = torch::constant_pad_nd(x1, {diffX / 2, diffX - diffX / 2, diffY / 2, diffY - diffY / 2}, 0);

        torch::Tensor x = torch::cat({x2, x1}, 1);

        //        x = dc(x);
        x = conv1(x);
        x = conv2(x);
        return x;
    }
    //    torch::nn::up
    //    DoubleConv dc;
    GatedConv conv1, conv2;
};
TORCH_MODULE(Up);

struct OutConvImpl : torch::nn::Module
{
    OutConvImpl(int in_channels, int out_channels)
        : conv1(torch::nn::Conv2dOptions(in_channels, out_channels, /*kernel_size=*/1))
    {
        register_module("conv1", conv1);
    }


    torch::Tensor forward(torch::Tensor x)
    {
        x = conv1(x);
        return x;
    }
    torch::nn::Conv2d conv1;
};
TORCH_MODULE(OutConv);

inline int k = 4;
struct UnetImpl : torch::nn::Module
{

    void _initialize_weights(){
        for (auto& module : modules(/*include_self=*/false)) {
            if (auto M = dynamic_cast<torch::nn::Conv2dImpl*>(module.get())) {
              torch::nn::init::kaiming_normal_(
                  M->weight,
                  /*a=*/0,
                  torch::kFanOut,
                  torch::kReLU);
              torch::nn::init::constant_(M->bias, 0);
            } else if (
                auto M = dynamic_cast<torch::nn::BatchNorm2dImpl*>(module.get())) {
              torch::nn::init::constant_(M->weight, 1);
              torch::nn::init::constant_(M->bias, 0);
            } else if (auto M = dynamic_cast<torch::nn::LinearImpl*>(module.get())) {
              torch::nn::init::normal_(M->weight, 0, 0.01);
              torch::nn::init::constant_(M->bias, 0);
            }
        }
    }

    UnetImpl(bool initialize_weights = true)
        : inconv(3, 64 / k),
          down1(64 / k, 128 / k),
          down2(128 / k, 256 / k),
          down3(256 / k, 512 / k),
          down4(512 / k, 512 / k),
          up1(1024 / k, 256 / k),
          up2(512 / k, 128 / k),
          up3(256 / k, 64 / k),
          up4(128 / k, 64 / k),
          outconv(64 / k, 3)
    {
        register_module("inconv", inconv);
        register_module("down1", down1);
        register_module("down2", down2);
        register_module("down3", down3);
        register_module("down4", down4);
        register_module("up1", up1);
        register_module("up2", up2);
        register_module("up3", up3);
        register_module("up4", up4);
        register_module("outconv", outconv);

        if (initialize_weights)
            _initialize_weights();
    }

    torch::Tensor forward(torch::Tensor x)
    {
        torch::Tensor x1 = inconv(x);
        torch::Tensor x2 = down1(x1);
        torch::Tensor x3 = down2(x2);
        torch::Tensor x4 = down3(x3);
        torch::Tensor x5 = down4(x4);
        x                = up1(x5, x4);
        x                = up2(x, x3);
        x                = up3(x, x2);
        x                = up4(x, x1);
        x                = outconv(x);
        return x;
    }

    DoubleConv inconv;
    Down down1, down2, down3, down4;
    Up up1, up2, up3, up4;
    OutConv outconv;

};
TORCH_MODULE(Unet);
