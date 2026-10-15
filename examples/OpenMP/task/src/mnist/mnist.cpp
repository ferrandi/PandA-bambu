/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */

#include "mnistInclude.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#include <vector>

// Training dataset path
const char* TRAIN_DS = std::getenv("BAMBU_MNIST_TRAIN");
// Inference dataset path
const char* INFERENCE_DS = std::getenv("BAMBU_MNIST_TEST");
// Model weights checkpoint path (train will save weights here, inference will load weights from here)
const char* MODEL_WEIGHTS = std::getenv("BAMBU_MNIST_MODEL");

enum class Device
{
   Host = 0,
   Accelerator = 1
};

void setMatrixParams(Matrix mtx[], const unsigned size, const int rows, const int cols)
{
   for(unsigned i = 0; i < size; i++)
   {
      mtx[i].rows = rows;
      mtx[i].cols = cols;
      std::memset(&(mtx[i].entries), 0, sizeof(Real) * MAX_BUFF_SIZE);
   }
}

#if TRAIN == 0
Real inference_kernel(Device device, NeuralNetwork* net, Img* imgs, std::vector<Real>& outputs)
{
   // Allocate buffer for flatten inputs
   Matrix flatten_imgs[INFERENCE_SAMPLES];
   setMatrixParams(flatten_imgs, INFERENCE_SAMPLES, IMG_SIZE * IMG_SIZE, 1);
   bool correct[INFERENCE_SAMPLES];

   // Recover model shapes
   Pair shapes[NUM_LAYERS];
   network_predict_imgs_get_shapes(net, &imgs[0], &flatten_imgs[0], shapes);

   // Allocate buffer for model intermediate tensors
   Matrix out1[INFERENCE_SAMPLES];
   Matrix out2[INFERENCE_SAMPLES];
   Matrix out3[INFERENCE_SAMPLES];
   Matrix out4[INFERENCE_SAMPLES];
   Matrix out5[INFERENCE_SAMPLES];
   setMatrixParams(out1, INFERENCE_SAMPLES, shapes[0].first, shapes[0].second);
   setMatrixParams(out2, INFERENCE_SAMPLES, shapes[1].first, shapes[1].second);
   setMatrixParams(out3, INFERENCE_SAMPLES, shapes[2].first, shapes[2].second);
   setMatrixParams(out4, INFERENCE_SAMPLES, shapes[3].first, shapes[3].second);
   setMatrixParams(out5, INFERENCE_SAMPLES, shapes[4].first, shapes[4].second);

   if(device == Device::Host)
   {
      fprintf(stderr, "Resources allocated, starting inference kernel on host\n");
      fn_host(net->hidden_weights, net->output_weights, imgs, flatten_imgs, correct, out1, out2, out3, out4, out5);
   }
   else
   {
      fprintf(stderr, "Resources allocated, starting inference kernel on accelerator\n");

#ifdef __BAMBU_SIM__
      m_param_alloc(0, sizeof(Matrix));
      m_param_alloc(1, sizeof(Matrix));
      m_param_alloc(2, sizeof(Img) * INFERENCE_SAMPLES);
      m_param_alloc(3, sizeof(flatten_imgs));
      m_param_alloc(4, sizeof(correct));
      m_param_alloc(5, sizeof(out1));
      m_param_alloc(6, sizeof(out2));
      m_param_alloc(7, sizeof(out3));
      m_param_alloc(8, sizeof(out4));
      m_param_alloc(9, sizeof(out5));
#endif
      fn(net->hidden_weights, net->output_weights, imgs, flatten_imgs, correct, out1, out2, out3, out4, out5);
   }

   outputs.clear();
   unsigned n_correct = 0;
   for(unsigned i = 0; i < INFERENCE_SAMPLES; i++)
   {
      n_correct += correct[i] == true;
      for(int entry = 0; entry < out5[i].rows * out5[i].cols; ++entry)
      {
         outputs.push_back(out5[i].entries[entry]);
      }
   }
   const Real score = static_cast<Real>(n_correct) / INFERENCE_SAMPLES;

   return score;
}

#else

Real train_kernel(Device device, NeuralNetwork* net, Img* imgs, bool save_weights_mat, Matrix** save_hidden_w,
                  Matrix** save_output_w, bool save_model_weights, const unsigned data_size = TRAINING_SAMPLES)
{
   Matrix* cached_hidden_w = matrix_copy(net->hidden_weights);
   Matrix* cached_output_w = matrix_copy(net->output_weights);
   if(save_weights_mat)
   {
      *save_hidden_w = matrix_copy(net->hidden_weights);
      *save_output_w = matrix_copy(net->output_weights);
   }

   // Retrieve network layers shapes
   std::vector<Pair> shapes;
   network_train_batch_imgs_get_shapes(net, imgs, shapes);

   // Restore original wieghts
   matrix_free(net->hidden_weights);
   matrix_free(net->output_weights);
   net->hidden_weights = cached_hidden_w;
   net->output_weights = cached_output_w;
   Matrix* flatten_imgs = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* hidden_w_acc = (Matrix*)malloc(sizeof(Matrix));
   Matrix* output_w_acc = (Matrix*)malloc(sizeof(Matrix));
   Matrix* output = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out1 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out2 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out3 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out4 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out5 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out6 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out7 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out8 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out9 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out10 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out11 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out12 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out13 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out15 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out16 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out17 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out18 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out19 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
   Matrix* out20 = (Matrix*)malloc(sizeof(Matrix) * BATCH_TRAIN_SAMPLES);

   setMatrixParams(flatten_imgs, BATCH_TRAIN_SAMPLES, IMG_SIZE * IMG_SIZE, 1);
   setMatrixParams(hidden_w_acc, 1, net->hidden_weights->rows, net->hidden_weights->cols);
   setMatrixParams(output_w_acc, 1, net->output_weights->rows, net->output_weights->cols);
   setMatrixParams(output, BATCH_TRAIN_SAMPLES, NUM_CLASSES, 1);
   setMatrixParams(out1, BATCH_TRAIN_SAMPLES, shapes[0].first, shapes[0].second);
   setMatrixParams(out2, BATCH_TRAIN_SAMPLES, shapes[1].first, shapes[1].second);
   setMatrixParams(out3, BATCH_TRAIN_SAMPLES, shapes[2].first, shapes[2].second);
   setMatrixParams(out4, BATCH_TRAIN_SAMPLES, shapes[3].first, shapes[3].second);
   setMatrixParams(out5, BATCH_TRAIN_SAMPLES, shapes[4].first, shapes[4].second);
   setMatrixParams(out6, BATCH_TRAIN_SAMPLES, shapes[5].first, shapes[5].second);
   setMatrixParams(out7, BATCH_TRAIN_SAMPLES, shapes[6].first, shapes[6].second);
   setMatrixParams(out8, BATCH_TRAIN_SAMPLES, shapes[7].first, shapes[7].second);
   setMatrixParams(out9, BATCH_TRAIN_SAMPLES, shapes[8].first, shapes[8].second);
   setMatrixParams(out10, BATCH_TRAIN_SAMPLES, shapes[9].first, shapes[9].second);
   setMatrixParams(out11, BATCH_TRAIN_SAMPLES, shapes[10].first, shapes[10].second);
   setMatrixParams(out12, BATCH_TRAIN_SAMPLES, shapes[11].first, shapes[11].second);
   setMatrixParams(out13, BATCH_TRAIN_SAMPLES, shapes[12].first, shapes[12].second);
   setMatrixParams(out15, BATCH_TRAIN_SAMPLES, shapes[14].first, shapes[14].second);
   setMatrixParams(out16, BATCH_TRAIN_SAMPLES, shapes[15].first, shapes[15].second);
   setMatrixParams(out17, BATCH_TRAIN_SAMPLES, shapes[16].first, shapes[16].second);
   setMatrixParams(out18, BATCH_TRAIN_SAMPLES, shapes[17].first, shapes[17].second);
   setMatrixParams(out19, BATCH_TRAIN_SAMPLES, shapes[18].first, shapes[18].second);
   setMatrixParams(out20, BATCH_TRAIN_SAMPLES, shapes[19].first, shapes[19].second);

   fprintf(stderr, "Starting training\n");

   Real loss = 0;
   for(unsigned batch = 0; batch < data_size; batch += BATCH_TRAIN_SAMPLES)
   {
      fprintf(stderr, "Batch: %d\n", batch / BATCH_TRAIN_SAMPLES);
      // Launch kernel
      if(device == Device::Host)
      {
         fn_host(hidden_w_acc, output_w_acc, net->hidden_weights, net->output_weights, &imgs[batch], flatten_imgs,
                 output, out1, out2, out3, out4, out5, out6, out7, out8, out9, out10, out11, out12, out13, out15, out16,
                 out17, out18, out19, out20);

#if COMPUTED_ACCUMULATOR_OUTSIDE_KERNEL == 1
         // Update accumulators
         for(unsigned i = 0; i < BATCH_TRAIN_SAMPLES; ++i)
         {
            add(output_w_acc, &out13[i], output_w_acc);
            add(hidden_w_acc, &out20[i], hidden_w_acc);
         }
#endif
      }
      else
      {
#ifdef __BAMBU_SIM__
         m_param_alloc(0, sizeof(Matrix));
         m_param_alloc(1, sizeof(Matrix));
         m_param_alloc(2, sizeof(Matrix));
         m_param_alloc(3, sizeof(Matrix));
         m_param_alloc(4, sizeof(Img) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(5, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(6, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(7, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(8, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(9, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(10, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(11, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(12, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(13, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(14, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(15, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(16, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(17, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(18, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(19, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(20, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(21, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(22, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(23, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(24, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
         m_param_alloc(25, sizeof(Matrix) * BATCH_TRAIN_SAMPLES);
#endif

         fn(hidden_w_acc, output_w_acc, net->hidden_weights, net->output_weights, &imgs[batch], flatten_imgs, output,
            out1, out2, out3, out4, out5, out6, out7, out8, out9, out10, out11, out12, out13, out15, out16, out17,
            out18, out19, out20);

#if COMPUTED_ACCUMULATOR_OUTSIDE_KERNEL == 1
         // Update accumulators
         for(unsigned i = 0; i < BATCH_TRAIN_SAMPLES; ++i)
         {
            add(output_w_acc, &out13[i], output_w_acc);
            add(hidden_w_acc, &out20[i], hidden_w_acc);
         }
#endif
      }

      loss = sum_square_errors(&out5[BATCH_TRAIN_SAMPLES - 1]);

      // for (unsigned k=0; k<BATCH_TRAIN_SAMPLES; k++)
      // {
      //    loss += sum_square_errors(&out2[k]);
      // }

      // Update weights
      add(net->hidden_weights, hidden_w_acc, net->hidden_weights);
      add(net->output_weights, output_w_acc, net->output_weights);

      setMatrixParams(flatten_imgs, BATCH_TRAIN_SAMPLES, IMG_SIZE * IMG_SIZE, 1);
      setMatrixParams(hidden_w_acc, 1, net->hidden_weights->rows, net->hidden_weights->cols);
      setMatrixParams(output_w_acc, 1, net->output_weights->rows, net->output_weights->cols);
      setMatrixParams(output, BATCH_TRAIN_SAMPLES, NUM_CLASSES, 1);
      setMatrixParams(out1, BATCH_TRAIN_SAMPLES, shapes[0].first, shapes[0].second);
      setMatrixParams(out2, BATCH_TRAIN_SAMPLES, shapes[1].first, shapes[1].second);
      setMatrixParams(out3, BATCH_TRAIN_SAMPLES, shapes[2].first, shapes[2].second);
      setMatrixParams(out4, BATCH_TRAIN_SAMPLES, shapes[3].first, shapes[3].second);
      setMatrixParams(out5, BATCH_TRAIN_SAMPLES, shapes[4].first, shapes[4].second);
      setMatrixParams(out6, BATCH_TRAIN_SAMPLES, shapes[5].first, shapes[5].second);
      setMatrixParams(out7, BATCH_TRAIN_SAMPLES, shapes[6].first, shapes[6].second);
      setMatrixParams(out8, BATCH_TRAIN_SAMPLES, shapes[7].first, shapes[7].second);
      setMatrixParams(out9, BATCH_TRAIN_SAMPLES, shapes[8].first, shapes[8].second);
      setMatrixParams(out10, BATCH_TRAIN_SAMPLES, shapes[9].first, shapes[9].second);
      setMatrixParams(out11, BATCH_TRAIN_SAMPLES, shapes[10].first, shapes[10].second);
      setMatrixParams(out12, BATCH_TRAIN_SAMPLES, shapes[11].first, shapes[11].second);
      setMatrixParams(out13, BATCH_TRAIN_SAMPLES, shapes[12].first, shapes[12].second);
      setMatrixParams(out15, BATCH_TRAIN_SAMPLES, shapes[14].first, shapes[14].second);
      setMatrixParams(out16, BATCH_TRAIN_SAMPLES, shapes[15].first, shapes[15].second);
      setMatrixParams(out17, BATCH_TRAIN_SAMPLES, shapes[16].first, shapes[16].second);
      setMatrixParams(out18, BATCH_TRAIN_SAMPLES, shapes[17].first, shapes[17].second);
      setMatrixParams(out19, BATCH_TRAIN_SAMPLES, shapes[18].first, shapes[18].second);
      setMatrixParams(out20, BATCH_TRAIN_SAMPLES, shapes[19].first, shapes[19].second);

      fprintf(stderr, "Loss at batch %d: %.5f\n", batch / BATCH_TRAIN_SAMPLES, loss);
   }

   if(save_model_weights)
   {
      network_save(net, MODEL_WEIGHTS);
   }

   free(flatten_imgs);
   free(hidden_w_acc);
   free(output_w_acc);
   free(output);
   free(out1);
   free(out2);
   free(out3);
   free(out4);
   free(out5);
   free(out6);
   free(out7);
   free(out8);
   free(out9);
   free(out10);
   free(out11);
   free(out12);
   free(out13);
   free(out15);
   free(out16);
   free(out17);
   free(out18);
   free(out19);
   free(out20);

   return loss;
}

#endif

// TODO: fix inference buff allocation
// TODO: imgs buffer allocation
int main()
{
#if FAKE_DATASET == 0
#if TRAIN == 1
   if(!TRAIN_DS)
   {
      std::fprintf(stderr, "Set BAMBU_MNIST_TRAIN to the training CSV path.\n");
      return 1;
   }
#else
   if(!INFERENCE_DS || !MODEL_WEIGHTS)
   {
      std::fprintf(stderr, "Set BAMBU_MNIST_TEST and BAMBU_MNIST_MODEL for inference.\n");
      return 1;
   }
#endif
#endif

   srand(42); // Fixing the seed for reproducibility

   NeuralNetwork* net = nullptr;
   Img* imgs = nullptr;

   bool error = false;

#if TRAIN == 1
   imgs = (Img*)malloc(sizeof(Img) * TRAINING_SAMPLES);
   for(unsigned i = 0; i < TRAINING_SAMPLES; i++)
   {
      std::memset(&(imgs[i].img_data.entries), 0, sizeof(Real) * MAX_BUFF_SIZE);
   }

   Real accelerator_batch_loss = 0.0;
   Real host_batch_loss = 0.0;
   Matrix* net_hidden_w = nullptr;
   Matrix* net_output_w = nullptr;
   csv_to_imgs(TRAIN_DS, imgs, TRAINING_SAMPLES);
   net = network_create(IMG_SIZE * IMG_SIZE, HIDDEN_LAYER_SIZE, NUM_CLASSES, LR);

   accelerator_batch_loss = train_kernel(Device::Accelerator, net, imgs, true, &net_hidden_w, &net_output_w, false);
   matrix_copy(net_hidden_w, net->hidden_weights);
   matrix_copy(net_output_w, net->output_weights);
   host_batch_loss = train_kernel(Device::Host, net, imgs, false, &net_hidden_w, &net_output_w, false);

   matrix_free(net->hidden_weights);
   matrix_free(net->output_weights);
   net->hidden_weights = net_hidden_w;
   net->output_weights = net_output_w;

   // Verify with original impl
   // original_kernel_loss = network_train_batch_imgs(net, imgs);

   fprintf(stderr, "Accelerator batch loss: %.5f\n", accelerator_batch_loss);
   fprintf(stderr, "Host batch loss: %.5f\n", host_batch_loss);
   // fprintf(stderr, "Original kernel loss (not batched): %.5f\n", original_kernel_loss);
   if(!std::isfinite(accelerator_batch_loss) || !std::isfinite(host_batch_loss) ||
      std::abs(accelerator_batch_loss - host_batch_loss) > EPSILON)
   {
      fprintf(stderr, "Loss does not match. Host: %.5f, device: %.5f\n", host_batch_loss, accelerator_batch_loss);
      error = true;
   }
   else
   {
      fprintf(stderr, "Loss between host and accelerator match\n");
   }

#else

   imgs = (Img*)malloc(sizeof(Img) * INFERENCE_SAMPLES);
   for(unsigned i = 0; i < INFERENCE_SAMPLES; i++)
   {
      std::memset(&(imgs[i].img_data.entries), 0, sizeof(Real) * MAX_BUFF_SIZE);
   }

   fprintf(stderr, "Running inference on %d samples...\n", INFERENCE_SAMPLES);
   csv_to_imgs(INFERENCE_DS, imgs, INFERENCE_SAMPLES);
   for(unsigned i = 0; i < INFERENCE_SAMPLES; i++)
   {
      fprintf(stderr, "Img %d ss %.5f\n", i, sum_square_errors(&imgs[i].img_data));
   }
   net = network_load(MODEL_WEIGHTS);

   std::vector<Real> accelerator_outputs;
   std::vector<Real> host_outputs;
   float score_accelerator = inference_kernel(Device::Accelerator, net, imgs, accelerator_outputs);
   float score_host = inference_kernel(Device::Host, net, imgs, host_outputs);

   if(accelerator_outputs.size() != host_outputs.size() || host_outputs.empty())
   {
      fprintf(stderr, "Inference output dimensions do not match.\n");
      error = true;
   }
   else
   {
      for(unsigned i = 0; i < host_outputs.size(); ++i)
      {
         if(!std::isfinite(accelerator_outputs[i]) || !std::isfinite(host_outputs[i]) ||
            std::abs(accelerator_outputs[i] - host_outputs[i]) > EPSILON * (1 + std::abs(host_outputs[i])))
         {
            fprintf(stderr, "Inference output %u does not match.\n", i);
            error = true;
         }
      }
   }

   fprintf(stderr, "Accelerator inference score: %.5f\n", score_accelerator);
   fprintf(stderr, "Host inference score: %.5f\n", score_host);

   if(std::abs(score_accelerator - score_host) > EPSILON)
   {
      fprintf(stderr, "Score does not match. Host: %.5f, device: %.5f\n", score_host, score_accelerator);
      error = true;
   }
   else
   {
      fprintf(stderr, "Score match\n");
   }
#endif

   network_free(net);
   free(imgs);

   return error;
}
