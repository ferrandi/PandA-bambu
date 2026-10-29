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
#ifndef MNIST_INCLUDE_HPP
#define MNIST_INCLUDE_HPP

#include <vector>

#define Real float

#define MAXCHAR 10000
#define NUM_LAYERS 5
#define EPSILON 1e-4
#define LR 0.01
#define BATCH_LR LR / static_cast<Real>(BATCH_TRAIN_SAMPLES)

#if defined(USE_REAL_DATASET)
#if !defined(USE_REAL_SIZES)
#error "USE_REAL_SIZES must be defined to use a real dataset"
#endif
#define FAKE_DATASET 0
#else
#define FAKE_DATASET 1
#endif

#if defined(USE_REAL_SIZES)
#define IMG_SIZE 28
#define BATCH_TRAIN_SAMPLES 16 // Max is 60k
#define TRAINING_SAMPLES BATCH_TRAIN_SAMPLES * 1
#define INFERENCE_SAMPLES 16
#define HIDDEN_LAYER_SIZE 300
#define NUM_CLASSES 10
#else
#define IMG_SIZE 5
#define BATCH_TRAIN_SAMPLES 4 // Max is 60k
#define TRAINING_SAMPLES BATCH_TRAIN_SAMPLES * 2
#define INFERENCE_SAMPLES 4
#define HIDDEN_LAYER_SIZE 32
#define NUM_CLASSES 10
#endif

#ifndef TRAIN
#define TRAIN 1
#endif

#ifndef COMPUTED_ACCUMULATOR_OUTSIDE_KERNEL
#define COMPUTED_ACCUMULATOR_OUTSIDE_KERNEL 1
#endif

#define USE_APPROXIMATE_EXP 0

#if IMG_SIZE == 28 && NUM_CLASSES != 10
#error "Wrong configuration"
#endif

#define MAX_BUFF_SIZE IMG_SIZE* IMG_SIZE* HIDDEN_LAYER_SIZE

typedef struct
{
   Real entries[MAX_BUFF_SIZE];
   int rows;
   int cols;
} Matrix;

typedef struct
{
   int input;
   int hidden;
   int output;
   Real learning_rate;
   Matrix* hidden_weights;
   Matrix* output_weights;
} NeuralNetwork;

typedef struct
{
   Matrix img_data;
   int label;
} Img;

typedef struct Pair
{
   int first;
   int second;
} Pair;

void csv_to_imgs(char const* file_string, Img* imgs, int number_of_imgs);
Matrix* add(Matrix* m1, Matrix* m2);
void add(Matrix* m1, Matrix* m2, Matrix* m);
Matrix* addScalar(Real n, Matrix* m);
Matrix* apply(Real (*func)(Real), Matrix* m);
Matrix* dot(Matrix* m1, Matrix* m2);
Matrix* matrix_copy(Matrix* m);
Matrix* matrix_create(int row, int col);
Matrix* matrix_load(char const* file_string);
Matrix* multiply(Matrix* m1, Matrix* m2);
void multiply(Matrix* m1, Matrix* m2, Matrix* m);
void set_output(Img* img, Matrix* out);
Matrix* scale(Real n, Matrix* m);
Matrix* sigmoidPrime(Matrix* m);
void sigmoidPrime(Matrix* m, Matrix* prime);
Matrix* softmax(Matrix* m);
Matrix* subtract(Matrix* m1, Matrix* m2);
void subtract(Matrix* m1, Matrix* m2, Matrix* m);
template <bool Order = false>
void subtract_constant(Matrix* m, Matrix* subtracted, const Real k)
{
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         if constexpr(Order == true)
         {
            subtracted->entries[i * m->cols + j] = m->entries[i * m->cols + j] - k;
         }
         else
         {
            subtracted->entries[i * m->cols + j] = k - m->entries[i * m->cols + j];
         }
      }
   }
}
Matrix* transpose(Matrix* m);
void transpose(Matrix* m, Matrix* mat);
NeuralNetwork* network_create(int input, int hidden, int output, Real lr);
NeuralNetwork* network_load(char const* file_string);
Real fast_exp(Real x);
Real sigmoid(Real input);
Real sum_square_errors(Matrix* errors);
Real hard_sigmoid(Real x);
int matrix_argmax(Matrix* m);
void apply(Real (*func)(Real), Matrix* m, Matrix* dest);
void check_prediction(Img* img, Matrix* output, bool& correct);
void dot(Matrix* m1, Matrix* m2, Matrix* out);
void img_free(Img* img);
void img_free(Img* img);
void img_print(Img* img);
void imgs_free(Img** imgs, int n);
void matrix_copy(Matrix* m, Matrix* res);
void matrix_fill(Matrix* m, int n);
Matrix* matrix_flatten(Matrix* m, int axis);
void matrix_flatten(Img* m, int axis, Matrix* flatten);
void matrix_flatten(Matrix* m, int axis, Matrix* flatten);
void matrix_free(Matrix* m);
void matrix_print(Matrix* m);
void matrix_randomize(Matrix* m, int n);
void matrix_save(Matrix* m, char const* file_string);
void network_free(NeuralNetwork* net);
void network_predict(NeuralNetwork* net, Matrix* input_data, Matrix* out1, Matrix* out2, Matrix* out3, Matrix* out4,
                     Matrix* out5);
void network_predict_get_shapes(NeuralNetwork* net, Matrix* input_data, Pair* shapes);
void network_predict_img(NeuralNetwork* net, Img* img, Matrix* flatten_img, Matrix* out1, Matrix* out2, Matrix* out3,
                         Matrix* out4, Matrix* out5);
void network_predict_imgs_get_shapes(NeuralNetwork* net, Img* img, Matrix* flatten_img, Pair* shapes);
void network_print(NeuralNetwork* net);
void network_save(NeuralNetwork* net, char const* file_string);
void network_train(NeuralNetwork* net, Matrix* input_data, Matrix* output_data, Real* loss);
Real network_train_batch_imgs(NeuralNetwork* net, Img* imgs);
void network_train_batch_imgs_get_shapes(NeuralNetwork* net, Img* imgs, std::vector<Pair>& shapes);
void softmax(Matrix* m, Matrix* res);
#if TRAIN == 0
void fn(Matrix* in_weights, Matrix* out_weights, Img* imgs, Matrix* flatten_imgs, bool* correct, Matrix* out1,
        Matrix* out2, Matrix* out3, Matrix* out4, Matrix* out5);

void fn_host(Matrix* in_weights, Matrix* out_weights, Img* imgs, Matrix* flatten_imgs, bool* correct, Matrix* out1,
             Matrix* out2, Matrix* out3, Matrix* out4, Matrix* out5);
#else
void fn(Matrix* hidde_w_acc, Matrix* output_w_acc, Matrix* hidden_weights, Matrix* output_weights, Img* imgs,
        Matrix* flatten, Matrix* output, Matrix* out1, Matrix* out2, Matrix* out3, Matrix* out4, Matrix* out5,
        Matrix* out6, Matrix* out7, Matrix* out8, Matrix* out9, Matrix* out10, Matrix* out11, Matrix* out12,
        Matrix* out13, Matrix* out15, Matrix* out16, Matrix* out17, Matrix* out18, Matrix* out19, Matrix* out20);
void fn_host(Matrix* hidde_w_acc, Matrix* output_w_acc, Matrix* hidden_weights, Matrix* output_weights, Img* imgs,
             Matrix* flatten, Matrix* output, Matrix* out1, Matrix* out2, Matrix* out3, Matrix* out4, Matrix* out5,
             Matrix* out6, Matrix* out7, Matrix* out8, Matrix* out9, Matrix* out10, Matrix* out11, Matrix* out12,
             Matrix* out13, Matrix* out15, Matrix* out16, Matrix* out17, Matrix* out18, Matrix* out19, Matrix* out20);
#endif

#endif
