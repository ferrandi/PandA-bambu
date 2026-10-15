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

// Dimension assertions belong to the host regression. Synthesized kernels
// cannot call the C library's __assert_fail implementation.
#ifdef __SYNTHESIS__
#ifndef NDEBUG
#define NDEBUG
#endif
#endif
#include "mnistInclude.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>

#include <vector>

Real sum_square_errors(Matrix* errors)
{
   Real sum = 0.0;
   for(int i = 0; i < errors->rows; i++)
   {
      for(int j = 0; j < errors->cols; j++)
      {
         Real err = errors->entries[i * errors->cols + j];
         sum += err * err;
      }
   }
   return sum;
}

Matrix* matrix_create(int row, int col)
{
   Matrix* matrix = nullptr;
   matrix = (Matrix*)malloc(sizeof(Matrix));
   matrix->rows = row;
   matrix->cols = col;
   // matrix->entries = (Real*)malloc(row * col * sizeof(Real));
   memset(matrix->entries, 0, sizeof(Real) * row * col);
   return matrix;
}

void matrix_fill(Matrix* m, int n)
{
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         m->entries[i * m->cols + j] = n;
      }
   }
}

void matrix_print(Matrix* m)
{
   fprintf(stderr, "Rows: %d Columns: %d\n", m->rows, m->cols);
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         fprintf(stderr, "%1.3f ", m->entries[i * m->cols + j]);
      }
      fprintf(stderr, "\n");
   }
}

/// 5th order Taylor series approximation
Real fast_exp(Real x)
{
   return 1.0f + x * (1.0f + x * (0.5f + x * (1.0f / 6.0f + x * (1.0f / 24.0f + x * (1.0f / 120.0f)))));
}

void matrix_copy(Matrix* m, Matrix* res)
{
#ifndef NDEBUG
   assert(m->rows == res->rows && m->cols == res->cols);
#endif
   const auto rows = m->rows;
   const auto cols = m->cols;
   for(int i = 0; i < rows; i++)
   {
      for(int j = 0; j < cols; j++)
      {
         res->entries[i * cols + j] = m->entries[i * cols + j];
      }
   }
}

Matrix* matrix_copy(Matrix* m)
{
   Matrix* mat = matrix_create(m->rows, m->cols);
   const auto rows = m->rows;
   const auto cols = m->cols;
   for(int i = 0; i < rows; i++)
   {
      for(int j = 0; j < cols; j++)
      {
         mat->entries[i * cols + j] = m->entries[i * cols + j];
      }
   }
   return mat;
}

void matrix_save(Matrix* m, char const* file_string)
{
   FILE* file = fopen(file_string, "w");
   fprintf(file, "%d\n", m->rows);
   fprintf(file, "%d\n", m->cols);
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         fprintf(file, "%.6f\n", m->entries[i * m->cols + j]);
      }
   }
   fprintf(stderr, "Successfully saved matrix to %s\n", file_string);
   fclose(file);
}

Matrix* matrix_load(char const* file_string)
{
   FILE* file = fopen(file_string, "r");
   char entry[MAXCHAR];
   fgets(entry, MAXCHAR, file);
   int rows = atoi(entry);
   fgets(entry, MAXCHAR, file);
   int cols = atoi(entry);
   Matrix* m = matrix_create(rows, cols);
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         fgets(entry, MAXCHAR, file);
         m->entries[i * m->cols + j] = strtod(entry, nullptr);
      }
   }
   fprintf(stderr, "Sucessfully loaded matrix from %s\n", file_string);
   fclose(file);
   return m;
}

Real uniform_distribution(Real low, Real high)
{
   Real difference = high - low; // The difference between the two
   int scale = 10000;
   int scaled_difference = (int)(difference * scale);
   return low + (1.0 * (rand() % scaled_difference) / scale);
}

void matrix_randomize(Matrix* m, int n)
{
   // Pulling from a random distribution of
   // Min: -1 / sqrt(n)
   // Max: 1 / sqrt(n)
   Real min = -1.0 / sqrt(n);
   Real max = 1.0 / sqrt(n);
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         m->entries[i * m->cols + j] = uniform_distribution(min, max);
      }
   }
}

int matrix_argmax(Matrix* m)
{
   // Expects a Mx1 matrix
   Real max_score = 0;
   int max_idx = 0;
   for(int i = 0; i < m->rows; i++)
   {
      if(m->entries[i * m->cols] > max_score)
      {
         max_score = m->entries[i * m->cols];
         max_idx = i;
      }
   }
   return max_idx;
}

// TODO: remove as we have flatten matrix representation in row major storage
void matrix_flatten(Img* m, int axis, Matrix* flatten)
{
   const auto m_rows = m->img_data.rows;
   const auto m_cols = m->img_data.cols;
   for(int i = 0; i < m_rows; i++)
   {
      for(int j = 0; j < m_cols; j++)
      {
         flatten->entries[i * m_cols + j] = m->img_data.entries[i * m_cols + j];
      }
   }
}

Matrix* matrix_flatten(Matrix* m, int axis)
{
   // Axis = 0 -> Column Vector, Axis = 1 -> Row Vector
   Matrix* flatten = matrix_create(m->rows * m->cols, 1);
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         flatten->entries[i * m->cols + j] = m->entries[i * m->cols + j];
      }
   }
   return flatten;
}

// TODO: remove as we have flatten matrix representation in row major storage
void matrix_flatten(Matrix* m, int axis, Matrix* flatten)
{
   // Axis = 0 -> Column Vector, Axis = 1 -> Row Vector
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         flatten->entries[i * m->cols + j] = m->entries[i * m->cols + j];
      }
   }
}

void matrix_free(Matrix* m)
{
   if(!m)
   {
      return;
   }
   free(m);
   m = nullptr;
}

int check_dimensions(Matrix* m1, Matrix* m2)
{
   if(m1->rows == m2->rows && m1->cols == m2->cols)
      return 1;
   return 0;
}

void multiply(Matrix* m1, Matrix* m2, Matrix* m)
{
#ifndef NDEBUG
   if(check_dimensions(m1, m2))
   {
#endif
      for(int i = 0; i < m1->rows; i++)
      {
         for(int j = 0; j < m2->cols; j++)
         {
            m->entries[i * m->cols + j] = m1->entries[i * m1->cols + j] * m2->entries[i * m2->cols + j];
         }
      }
#ifndef NDEBUG
   }
   else
   {
      fprintf(stderr, "Dimension mistmatch multiply: %dx%d %dx%d\n", m1->rows, m1->cols, m2->rows, m2->cols);
      exit(1);
   }
#endif
}

Matrix* multiply(Matrix* m1, Matrix* m2)
{
#ifndef NDEBUG
   if(check_dimensions(m1, m2))
   {
#endif
      Matrix* m = matrix_create(m1->rows, m1->cols);
      for(int i = 0; i < m1->rows; i++)
      {
         for(int j = 0; j < m2->cols; j++)
         {
            m->entries[i * m->cols + j] = m1->entries[i * m1->cols + j] * m2->entries[i * m2->cols + j];
         }
      }
      return m;
#ifndef NDEBUG
   }
   else
   {
      fprintf(stderr, "Dimension mistmatch multiply: %dx%d %dx%d\n", m1->rows, m1->cols, m2->rows, m2->cols);
      exit(1);
   }
#endif
}

void add(Matrix* m1, Matrix* m2, Matrix* m)
{
#ifndef NDEBUG
   if(check_dimensions(m1, m2))
   {
#endif
      for(int i = 0; i < m1->rows; i++)
      {
         for(int j = 0; j < m2->cols; j++)
         {
            m->entries[i * m->cols + j] = m1->entries[i * m1->cols + j] + m2->entries[i * m2->cols + j];
         }
      }
#ifndef NDEBUG
   }
   else
   {
      fprintf(stderr, "Dimension mistmatch add: %dx%d %dx%d\n", m1->rows, m1->cols, m2->rows, m2->cols);
      exit(1);
   }
#endif
}

Matrix* add(Matrix* m1, Matrix* m2)
{
#ifndef NDEBUG
   if(check_dimensions(m1, m2))
   {
#endif
      Matrix* m = matrix_create(m1->rows, m1->cols);
      for(int i = 0; i < m1->rows; i++)
      {
         for(int j = 0; j < m2->cols; j++)
         {
            m->entries[i * m->cols + j] = m1->entries[i * m1->cols + j] + m2->entries[i * m2->cols + j];
         }
      }
      return m;
#ifndef NDEBUG
   }
   else
   {
      fprintf(stderr, "Dimension mistmatch add: %dx%d %dx%d\n", m1->rows, m1->cols, m2->rows, m2->cols);
      exit(1);
   }
#endif
}

void subtract(Matrix* m1, Matrix* m2, Matrix* m)
{
#ifndef NDEBUG
   if(check_dimensions(m1, m2))
   {
#endif
      for(int i = 0; i < m1->rows; i++)
      {
         for(int j = 0; j < m2->cols; j++)
         {
            m->entries[i * m->cols + j] = m1->entries[i * m1->cols + j] - m2->entries[i * m2->cols + j];
         }
      }
#ifndef NDEBUG
   }
   else
   {
      fprintf(stderr, "Dimension mistmatch subtract: %dx%d %dx%d\n", m1->rows, m1->cols, m2->rows, m2->cols);
      exit(1);
   }
#endif
}

Matrix* subtract(Matrix* m1, Matrix* m2)
{
#ifndef NDEBUG
   if(check_dimensions(m1, m2))
   {
#endif
      Matrix* m = matrix_create(m1->rows, m1->cols);
      for(int i = 0; i < m1->rows; i++)
      {
         for(int j = 0; j < m2->cols; j++)
         {
            m->entries[i * m->cols + j] = m1->entries[i * m1->cols + j] - m2->entries[i * m2->cols + j];
         }
      }
      return m;
#ifndef NDEBUG
   }
   else
   {
      fprintf(stderr, "Dimension mistmatch subtract: %dx%d %dx%d\n", m1->rows, m1->cols, m2->rows, m2->cols);
      exit(1);
   }
#endif
}

void apply(Real (*func)(Real), Matrix* m, Matrix* dest)
{
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         dest->entries[i * dest->cols + j] = (*func)(m->entries[i * m->cols + j]);
      }
   }
}

Matrix* apply(Real (*func)(Real), Matrix* m)
{
   Matrix* mat = matrix_copy(m);
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         mat->entries[i * mat->cols + j] = (*func)(m->entries[i * m->cols + j]);
      }
   }
   return mat;
}

void dot(Matrix* m1, Matrix* m2, Matrix* out)
{
   if(m1->cols == m2->rows)
   {
      for(int i = 0; i < m1->rows; i++)
      {
         for(int j = 0; j < m2->cols; j++)
         {
            Real sum = 0;
            for(int k = 0; k < m2->rows; k++)
            {
               sum += m1->entries[i * m1->cols + k] * m2->entries[k * m2->cols + j];
            }
            out->entries[i * out->cols + j] = sum;
         }
      }
   }
}

Matrix* dot(Matrix* m1, Matrix* m2)
{
#ifndef NDEBUG
   if(m1->cols == m2->rows)
   {
#endif
      Matrix* m = matrix_create(m1->rows, m2->cols);
      for(int i = 0; i < m1->rows; i++)
      {
         for(int j = 0; j < m2->cols; j++)
         {
            Real sum = 0;
            for(int k = 0; k < m2->rows; k++)
            {
               sum += m1->entries[i * m1->cols + k] * m2->entries[k * m2->cols + j];
            }
            m->entries[i * m->cols + j] = sum;
         }
      }
      return m;
#ifndef NDEBUG
   }
   else
   {
      exit(1);
   }
#endif
}

void scale(Real n, Matrix* m, Matrix* mat)
{
   matrix_copy(m, mat);
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         mat->entries[i * mat->cols + j] *= n;
      }
   }
}

Matrix* scale(Real n, Matrix* m)
{
   Matrix* mat = matrix_copy(m);
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         mat->entries[i * mat->cols + j] *= n;
      }
   }
   return mat;
}

Matrix* addScalar(Real n, Matrix* m)
{
   Matrix* mat = matrix_copy(m);
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         mat->entries[i * mat->cols + j] += n;
      }
   }
   return mat;
}

void transpose(Matrix* m, Matrix* mat)
{
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         mat->entries[j * mat->cols + i] = m->entries[i * m->cols + j];
      }
   }
}

Matrix* transpose(Matrix* m)
{
   Matrix* mat = matrix_create(m->cols, m->rows);
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
         mat->entries[j * mat->cols + i] = m->entries[i * m->cols + j];
      }
   }
   return mat;
}

Real hard_sigmoid(Real x)
{
   return std::max(0.0f, std::min(1.0f, 0.2f * x + 0.5f));
}

Real sigmoid(Real input)
{
#if USE_APPROXIMATE_EXP == 1
   return 1.0 / (1 + fast_exp(-1 * input));
#else
   return 1.0 / (1 + std::exp(-1 * input));
#endif
}

void sigmoidPrime(Matrix* m, Matrix* subtracted, Matrix* prime)
{
   subtract_constant<false>(m, subtracted, static_cast<Real>(1));
   multiply(m, subtracted, prime);
}

Matrix* sigmoidPrime(Matrix* m)
{
   Matrix* ones = matrix_create(m->rows, m->cols);
   matrix_fill(ones, 1);
   Matrix* subtracted = subtract(ones, m);
   Matrix* multiplied = multiply(m, subtracted);
   return multiplied;
}

Matrix* softmax(Matrix* m)
{
   Real total = 0;
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
#if USE_APPROXIMATE_EXP == 1
         total += fast_exp(m->entries[i * m->cols + j]);
#else
         total += std::exp(m->entries[i * m->cols + j]);
#endif
      }
   }
   Matrix* mat = matrix_create(m->rows, m->cols);
   for(int i = 0; i < mat->rows; i++)
   {
      for(int j = 0; j < mat->cols; j++)
      {
#if USE_APPROXIMATE_EXP == 1
         mat->entries[i * mat->cols + j] = fast_exp(m->entries[i * m->cols + j]) / total;
#else
         mat->entries[i * mat->cols + j] = std::exp(m->entries[i * m->cols + j]) / total;
#endif
      }
   }
   return mat;
}

void softmax(Matrix* m, Matrix* res)
{
   Real total = 0;
   for(int i = 0; i < m->rows; i++)
   {
      for(int j = 0; j < m->cols; j++)
      {
#if USE_APPROXIMATE_EXP == 1
         total += fast_exp(m->entries[i * m->cols + j]);
#else
         total += std::exp(m->entries[i * m->cols + j]);
#endif
      }
   }
   for(int i = 0; i < res->rows; i++)
   {
      for(int j = 0; j < res->cols; j++)
      {
#if USE_APPROXIMATE_EXP == 1
         res->entries[i * res->cols + j] = fast_exp(m->entries[i * m->cols + j]) / total;
#else
         res->entries[i * res->cols + j] = std::exp(m->entries[i * m->cols + j]) / total;
#endif
      }
   }
}

void network_free(NeuralNetwork* net)
{
   if(!net)
   {
      return;
   }
   matrix_free(net->hidden_weights);
   matrix_free(net->output_weights);
   free(net);
   net = nullptr;
}

NeuralNetwork* network_create(int input, int hidden, int output, Real lr)
{
   NeuralNetwork* net = (NeuralNetwork*)malloc(sizeof(NeuralNetwork));
   net->input = input;
   net->hidden = hidden;
   net->output = output;
   net->learning_rate = lr;
   Matrix* hidden_layer = matrix_create(hidden, input);
   Matrix* output_layer = matrix_create(output, hidden);
   matrix_randomize(hidden_layer, hidden);
   matrix_randomize(output_layer, output);
   net->hidden_weights = hidden_layer;
   net->output_weights = output_layer;
   return net;
}

void network_train_get_shapes(NeuralNetwork* net, Matrix* input, Matrix* output, Real* loss, std::vector<Pair>& shapes)
{
   // Feed forward
   Matrix* hidden_inputs = dot(net->hidden_weights, input);
   shapes.push_back({hidden_inputs->rows, hidden_inputs->cols});
   Matrix* hidden_outputs = apply(sigmoid, hidden_inputs);
   shapes.push_back({hidden_outputs->rows, hidden_outputs->cols});
   Matrix* final_inputs = dot(net->output_weights, hidden_outputs);
   shapes.push_back({final_inputs->rows, final_inputs->cols});
   Matrix* final_outputs = apply(sigmoid, final_inputs);
   shapes.push_back({final_outputs->rows, final_outputs->cols});

   // Find errors
   Matrix* output_errors = subtract(output, final_outputs);
   shapes.push_back({output_errors->rows, output_errors->cols});
   *loss = sum_square_errors(output_errors);
   Matrix* transposed_mat = transpose(net->output_weights);
   shapes.push_back({transposed_mat->rows, transposed_mat->cols});
   Matrix* hidden_errors = dot(transposed_mat, output_errors);
   shapes.push_back({hidden_errors->rows, hidden_errors->cols});
   matrix_free(transposed_mat);

   // Backpropogate
   // output_weights = add(
   //		 output_weights,
   //     scale(
   // 			  net->lr,
   //			  dot(
   // 		 			multiply(
   // 						output_errors,
   //				  	sigmoidPrime(final_outputs)
   //					),
   //					transpose(hidden_outputs)
   // 				)
   //		 )
   // )
   Matrix* sigmoid_primed_mat = sigmoidPrime(final_outputs);
   // sig prime sub
   shapes.push_back({final_outputs->rows, final_outputs->cols});
   // sig prime res
   shapes.push_back({sigmoid_primed_mat->rows, sigmoid_primed_mat->cols});
   Matrix* multiplied_mat = multiply(output_errors, sigmoid_primed_mat);
   shapes.push_back({multiplied_mat->rows, multiplied_mat->cols});
   transposed_mat = transpose(hidden_outputs);
   shapes.push_back({transposed_mat->rows, transposed_mat->cols});
   Matrix* dot_mat = dot(multiplied_mat, transposed_mat);
   shapes.push_back({dot_mat->rows, dot_mat->cols});
   Matrix* scaled_mat = scale(net->learning_rate, dot_mat);
   shapes.push_back({scaled_mat->rows, scaled_mat->cols});
   Matrix* added_mat = add(net->output_weights, scaled_mat);
   shapes.push_back({added_mat->rows, added_mat->cols});

   matrix_free(net->output_weights); // Free the old weights before replacing
   net->output_weights = added_mat;

   matrix_free(sigmoid_primed_mat);
   matrix_free(multiplied_mat);
   matrix_free(transposed_mat);
   matrix_free(dot_mat);
   matrix_free(scaled_mat);

   // hidden_weights = add(
   // 	 net->hidden_weights,
   // 	 scale (
   //			net->learning_rate
   //    	dot (
   //				multiply(
   //					hidden_errors,
   //					sigmoidPrime(hidden_outputs)
   //				)
   //				transpose(inputs)
   //      )
   // 	 )
   // )
   // Reusing variables after freeing memory
   sigmoid_primed_mat = sigmoidPrime(hidden_outputs);
   // for sig prime sub
   shapes.push_back({hidden_outputs->rows, hidden_outputs->cols});
   // for sig prime out
   shapes.push_back({sigmoid_primed_mat->rows, sigmoid_primed_mat->cols});
   multiplied_mat = multiply(hidden_errors, sigmoid_primed_mat);
   shapes.push_back({multiplied_mat->rows, multiplied_mat->cols});
   transposed_mat = transpose(input);
   shapes.push_back({transposed_mat->rows, transposed_mat->cols});
   dot_mat = dot(multiplied_mat, transposed_mat);
   shapes.push_back({dot_mat->rows, dot_mat->cols});
   scaled_mat = scale(net->learning_rate, dot_mat);
   shapes.push_back({scaled_mat->rows, scaled_mat->cols});
   added_mat = add(net->hidden_weights, scaled_mat);
   shapes.push_back({added_mat->rows, added_mat->cols});
   matrix_free(net->hidden_weights); // Free the old hidden_weights before replacement
   net->hidden_weights = added_mat;

   matrix_free(sigmoid_primed_mat);
   matrix_free(multiplied_mat);
   matrix_free(transposed_mat);
   matrix_free(dot_mat);
   matrix_free(scaled_mat);

   // Free matrices
   matrix_free(hidden_inputs);
   matrix_free(hidden_outputs);
   matrix_free(final_inputs);
   matrix_free(final_outputs);
   matrix_free(output_errors);
   matrix_free(hidden_errors);
}

Real network_train_batch_imgs(NeuralNetwork* net, Img* imgs)
{
   Real loss = 0.0;
   for(unsigned i = 0; i < TRAINING_SAMPLES; i++)
   {
      Img* cur_img = &imgs[i];
      Matrix* flatten = matrix_create(cur_img->img_data.rows * cur_img->img_data.cols, 1);
      matrix_flatten(&cur_img->img_data, 0, flatten);
      Matrix* output = matrix_create(NUM_CLASSES, 1);
      output->entries[cur_img->label] = static_cast<Real>(1); // Setting the result

      network_train(net, flatten, output, &loss);

      matrix_free(flatten);
      matrix_free(output);
   }
   return loss;
}

void network_train_batch_imgs_get_shapes(NeuralNetwork* net, Img* imgs, std::vector<Pair>& shapes)
{
   Real loss = 0.0;
   Img* cur_img = &imgs[0];
   Matrix* flatten = matrix_create(cur_img->img_data.rows * cur_img->img_data.cols, 1);
   matrix_flatten(&cur_img->img_data, 0, flatten);
   Matrix* output = matrix_create(NUM_CLASSES, 1);
   output->entries[cur_img->label] = static_cast<Real>(1); // Setting the result

   network_train_get_shapes(net, flatten, output, &loss, shapes);
}

/// Vanilla version
void network_train(NeuralNetwork* net, Matrix* input, Matrix* output, Real* loss)
{
   // Feed forward
   Matrix* hidden_inputs = dot(net->hidden_weights, input);
   Matrix* hidden_outputs = apply(sigmoid, hidden_inputs);
   Matrix* final_inputs = dot(net->output_weights, hidden_outputs);
   Matrix* final_outputs = apply(sigmoid, final_inputs);

   // Find errors
   Matrix* output_errors = subtract(output, final_outputs);
   *loss = sum_square_errors(output_errors);
   Matrix* transposed_mat = transpose(net->output_weights);
   Matrix* hidden_errors = dot(transposed_mat, output_errors);
   matrix_free(transposed_mat);

   // Backpropogate
   // output_weights = add(
   //		 output_weights,
   //     scale(
   // 			  net->lr,
   //			  dot(
   // 		 			multiply(
   // 						output_errors,
   //				  	sigmoidPrime(final_outputs)
   //					),
   //					transpose(hidden_outputs)
   // 				)
   //		 )
   // )
   Matrix* sigmoid_primed_mat = sigmoidPrime(final_outputs);
   Matrix* multiplied_mat = multiply(output_errors, sigmoid_primed_mat);
   transposed_mat = transpose(hidden_outputs);
   Matrix* dot_mat = dot(multiplied_mat, transposed_mat);
   Matrix* scaled_mat = scale(net->learning_rate, dot_mat);
   Matrix* added_mat = add(net->output_weights, scaled_mat);

   matrix_free(net->output_weights); // Free the old weights before replacing
   net->output_weights = added_mat;

   matrix_free(sigmoid_primed_mat);
   matrix_free(multiplied_mat);
   matrix_free(transposed_mat);
   matrix_free(dot_mat);
   matrix_free(scaled_mat);

   // hidden_weights = add(
   // 	 net->hidden_weights,
   // 	 scale (
   //			net->learning_rate
   //    	dot (
   //				multiply(
   //					hidden_errors,
   //					sigmoidPrime(hidden_outputs)
   //				)
   //				transpose(inputs)
   //      )
   // 	 )
   // )
   // Reusing variables after freeing memory
   sigmoid_primed_mat = sigmoidPrime(hidden_outputs);
   multiplied_mat = multiply(hidden_errors, sigmoid_primed_mat);
   transposed_mat = transpose(input);
   dot_mat = dot(multiplied_mat, transposed_mat);
   scaled_mat = scale(net->learning_rate, dot_mat);
   added_mat = add(net->hidden_weights, scaled_mat);
   matrix_free(net->hidden_weights); // Free the old hidden_weights before replacement
   net->hidden_weights = added_mat;

   matrix_free(sigmoid_primed_mat);
   matrix_free(multiplied_mat);
   matrix_free(transposed_mat);
   matrix_free(dot_mat);
   matrix_free(scaled_mat);

   // Free matrices
   matrix_free(hidden_inputs);
   matrix_free(hidden_outputs);
   matrix_free(final_inputs);
   matrix_free(final_outputs);
   matrix_free(output_errors);
   matrix_free(hidden_errors);
}

void set_output(Img* img, Matrix* out)
{
   out->entries[img->label] = static_cast<Real>(1); // Setting the result
}

#if TRAIN == 1
__attribute__((noinline)) void fn_host(Matrix* hidden_weights_accumulator, Matrix* output_weights_accumulator,
                                       Matrix* hidden_weights, Matrix* output_weights, Img* imgs, Matrix* flatten,
                                       Matrix* output, Matrix* out1, Matrix* out2, Matrix* out3, Matrix* out4,
                                       Matrix* out5, Matrix* out6, Matrix* out7, Matrix* out8, Matrix* out9,
                                       Matrix* out10, Matrix* out11, Matrix* out12, Matrix* out13, Matrix* out15,
                                       Matrix* out16, Matrix* out17, Matrix* out18, Matrix* out19, Matrix* out20)
{
   if(hidden_weights_accumulator && output_weights_accumulator && hidden_weights && output_weights && imgs && flatten &&
      output && out1 && out2 && out3 && out4 && out5 && out6 && out7 && out8 && out9 && out10 && out11 && out12 &&
      out13 && out15 && out16 && out17 && out18 && out19 && out20)
   {
      for(int i = 0; i < BATCH_TRAIN_SAMPLES; i++)
      {
         matrix_flatten(&imgs[i], 0, &flatten[i]);
         set_output(&imgs[i], &output[i]);
         dot(hidden_weights, &flatten[i], &out1[i]);
         apply(sigmoid, &out1[i], &out2[i]);
         dot(output_weights, &out2[i], &out3[i]);
         apply(sigmoid, &out3[i], &out4[i]);
         subtract(&output[i], &out4[i], &out5[i]);
         transpose(output_weights, &out6[i]);
         dot(&out6[i], &out5[i], &out7[i]);
         sigmoidPrime(&out4[i], &out8[i], &out9[i]);
         multiply(&out5[i], &out9[i], &out10[i]);
         transpose(&out2[i], &out11[i]);
         dot(&out10[i], &out11[i], &out12[i]);
         scale(static_cast<Real>(BATCH_LR), &out12[i], &out13[i]);
#if COMPUTED_ACCUMULATOR_OUTSIDE_KERNEL == 0
         add(output_weights_accumulator, &out13[i], output_weights_accumulator);
#endif
         sigmoidPrime(&out2[i], &out15[i], &out16[i]);
         multiply(&out7[i], &out16[i], &out17[i]);
         transpose(&flatten[i], &out18[i]);
         dot(&out17[i], &out18[i], &out19[i]);
         scale(static_cast<Real>(BATCH_LR), &out19[i], &out20[i]);
#if COMPUTED_ACCUMULATOR_OUTSIDE_KERNEL == 0
         add(hidden_weights_accumulator, &out20[i], hidden_weights_accumulator);
#endif
#if SHOW_LOSS == 1
         const Real loss = sum_square_errors(&out5[i]);
         if(i % SHOW_LOSS_SAMPLES == 0)
         {
            printf("Loss %.5f\n", loss);
         }
#endif
      }
   }
}

#if BANK_NUMBER == 2
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 2 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 2 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 2 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 2 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 2 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 2 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 2 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 2 chunk_size = 1024
#endif
#elif BANK_NUMBER == 32
#ifdef USE_AXI
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 32 chunk_size = 4 mode = m_axi
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 32 chunk_size = 16 mode = m_axi
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 32 chunk_size = 32 mode = m_axi
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 32 chunk_size = 64 mode = m_axi
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 32 chunk_size = 128 mode = m_axi
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 32 chunk_size = 256 mode = m_axi
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 32 chunk_size = 512 mode = m_axi
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 32 chunk_size = 1024 mode = m_axi
#endif
#else
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 32 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 32 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 32 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 32 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 32 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 32 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 32 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 32 chunk_size = 1024
#endif
#endif
#elif BANK_NUMBER == 8
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 8 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 8 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 8 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 8 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 8 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 8 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 8 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 8 chunk_size = 1024
#endif
#elif BANK_NUMBER == 16
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 16 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 16 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 16 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 16 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 16 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 16 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 16 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 16 chunk_size = 1024
#endif
#elif BANK_NUMBER == 4
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 4 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 4 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 4 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 4 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 4 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 4 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 4 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 4 chunk_size = 1024
#endif
#elif BANK_NUMBER == 64
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 64 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 64 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 64 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 64 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 64 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 64 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 64 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 64 chunk_size = 1024
#endif
#elif BANK_NUMBER == 128
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 128 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 128 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 128 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 128 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 128 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 128 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 128 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 128 chunk_size = 1024
#endif
#endif
#ifdef USE_AXI
#ifndef BANK_NUMBER
#pragma HLS bus mode = m_axi
#endif
#pragma HLS interface port = hidden_weights_accumulator mode = bus offset = direct
#pragma HLS interface port = output_weights_accumulator mode = bus offset = direct
#pragma HLS interface port = hidden_weights mode = bus offset = direct
#pragma HLS interface port = output_weights mode = bus offset = direct
#pragma HLS interface port = imgs mode = bus offset = direct
#pragma HLS interface port = flatten mode = bus offset = direct
#pragma HLS interface port = output mode = bus offset = direct
#pragma HLS interface port = out1 mode = bus offset = direct
#pragma HLS interface port = out2 mode = bus offset = direct
#pragma HLS interface port = out3 mode = bus offset = direct
#pragma HLS interface port = out4 mode = bus offset = direct
#pragma HLS interface port = out5 mode = bus offset = direct
#pragma HLS interface port = out6 mode = bus offset = direct
#pragma HLS interface port = out7 mode = bus offset = direct
#pragma HLS interface port = out8 mode = bus offset = direct
#pragma HLS interface port = out9 mode = bus offset = direct
#pragma HLS interface port = out10 mode = bus offset = direct
#pragma HLS interface port = out11 mode = bus offset = direct
#pragma HLS interface port = out12 mode = bus offset = direct
#pragma HLS interface port = out13 mode = bus offset = direct
#pragma HLS interface port = out15 mode = bus offset = direct
#pragma HLS interface port = out16 mode = bus offset = direct
#pragma HLS interface port = out17 mode = bus offset = direct
#pragma HLS interface port = out18 mode = bus offset = direct
#pragma HLS interface port = out19 mode = bus offset = direct
#pragma HLS interface port = out20 mode = bus offset = direct
#else
#pragma HLS interface port = hidden_weights_accumulator mode = bus
#pragma HLS interface port = output_weights_accumulator mode = bus
#pragma HLS interface port = hidden_weights mode = bus
#pragma HLS interface port = output_weights mode = bus
#pragma HLS interface port = imgs mode = bus
#pragma HLS interface port = flatten mode = bus
#pragma HLS interface port = output mode = bus
#pragma HLS interface port = out1 mode = bus
#pragma HLS interface port = out2 mode = bus
#pragma HLS interface port = out3 mode = bus
#pragma HLS interface port = out4 mode = bus
#pragma HLS interface port = out5 mode = bus
#pragma HLS interface port = out6 mode = bus
#pragma HLS interface port = out7 mode = bus
#pragma HLS interface port = out8 mode = bus
#pragma HLS interface port = out9 mode = bus
#pragma HLS interface port = out10 mode = bus
#pragma HLS interface port = out11 mode = bus
#pragma HLS interface port = out12 mode = bus
#pragma HLS interface port = out13 mode = bus
#pragma HLS interface port = out15 mode = bus
#pragma HLS interface port = out16 mode = bus
#pragma HLS interface port = out17 mode = bus
#pragma HLS interface port = out18 mode = bus
#pragma HLS interface port = out19 mode = bus
#pragma HLS interface port = out20 mode = bus
#endif
__attribute__((noinline)) void fn(Matrix* hidden_weights_accumulator, Matrix* output_weights_accumulator,
                                  Matrix* hidden_weights, Matrix* output_weights, Img* imgs, Matrix* flatten,
                                  Matrix* output, Matrix* out1, Matrix* out2, Matrix* out3, Matrix* out4, Matrix* out5,
                                  Matrix* out6, Matrix* out7, Matrix* out8, Matrix* out9, Matrix* out10, Matrix* out11,
                                  Matrix* out12, Matrix* out13, Matrix* out15, Matrix* out16, Matrix* out17,
                                  Matrix* out18, Matrix* out19, Matrix* out20)
{
#pragma omp parallel default(none)                                                                                     \
    shared(hidden_weights_accumulator, output_weights_accumulator, hidden_weights, output_weights, imgs, flatten,      \
               output, out1, out2, out3, out4, out5, out6, out7, out8, out9, out10, out11, out12, out13, out15, out16, \
               out17, out18, out19, out20) num_threads(1)
   {
#pragma omp single nowait
      {
         if(hidden_weights_accumulator && output_weights_accumulator && hidden_weights && output_weights && imgs &&
            flatten && output && out1 && out2 && out3 && out4 && out5 && out6 && out7 && out8 && out9 && out10 &&
            out11 && out12 && out13 && out15 && out16 && out17 && out18 && out19 && out20)
         {
            for(int i = 0; i < BATCH_TRAIN_SAMPLES; i++)
            {
#pragma omp task firstprivate(i) depend(inout : imgs[i], flatten[i])
               {
                  matrix_flatten(&imgs[i], 0, &flatten[i]);
               }
#pragma omp task firstprivate(i) depend(in : imgs[i]) depend(out : output[i])
               {
                  set_output(&imgs[i], &output[i]);
               }
               // Feed forward
#pragma omp task firstprivate(i) depend(in : hidden_weights, flatten[i]) depend(out : out1[i])
               {
                  dot(hidden_weights, &flatten[i], &out1[i]);
               }
#pragma omp task firstprivate(i) depend(in : out1[i]) depend(out : out2[i])
               {
                  apply(sigmoid, &out1[i], &out2[i]);
               }
#pragma omp task firstprivate(i) depend(in : output_weights, out2[i]) depend(out : out3[i])
               {
                  dot(output_weights, &out2[i], &out3[i]);
               }
#pragma omp task firstprivate(i) depend(in : out3[i]) depend(out : out4[i])
               {
                  apply(sigmoid, &out3[i], &out4[i]);
               }
               // Find errors
#pragma omp task firstprivate(i) depend(in : output[i], out4[i]) depend(out : out5[i])
               {
                  subtract(&output[i], &out4[i], &out5[i]);
               }
#pragma omp task firstprivate(i) depend(in : output_weights) depend(out : out6[i])
               {
                  transpose(output_weights, &out6[i]);
               }
#pragma omp task firstprivate(i) depend(in : out6[i], out5[i]) depend(out : out7[i])
               {
                  dot(&out6[i], &out5[i], &out7[i]);
               }
#pragma omp task firstprivate(i) depend(in : out4[i]) depend(out : out8[i], out9[i])
               {
                  sigmoidPrime(&out4[i], &out8[i], &out9[i]);
               }
#pragma omp task firstprivate(i) depend(in : out5[i], out9[i]) depend(out : out10[i])
               {
                  multiply(&out5[i], &out9[i], &out10[i]);
               }
#pragma omp task firstprivate(i) depend(in : out2[i]) depend(out : out11[i])
               {
                  transpose(&out2[i], &out11[i]);
               }
#pragma omp task firstprivate(i) depend(in : out10[i], out11[i]) depend(out : out12[i])
               {
                  dot(&out10[i], &out11[i], &out12[i]);
               }
#pragma omp task firstprivate(i) depend(in : out12[i]) depend(out : out13[i])
               {
                  scale(static_cast<Real>(BATCH_LR), &out12[i], &out13[i]);
               }
#if COMPUTED_ACCUMULATOR_OUTSIDE_KERNEL == 0
#pragma omp task firstprivate(i) depend(inout : output_weights_accumulator) depend(in : out13[i])
               {
                  add(output_weights_accumulator, &out13[i], output_weights_accumulator);
               }
#endif
#pragma omp task firstprivate(i) depend(in : out2[i]) depend(out : out15[i], out16[i])
               {
                  sigmoidPrime(&out2[i], &out15[i], &out16[i]);
               }
#pragma omp task firstprivate(i) depend(in : out7[i], out16[i]) depend(out : out17[i])
               {
                  multiply(&out7[i], &out16[i], &out17[i]);
               }
#pragma omp task firstprivate(i) depend(in : flatten[i]) depend(out : out18[i])
               {
                  transpose(&flatten[i], &out18[i]);
               }
#pragma omp task firstprivate(i) depend(in : out17[i], out18[i]) depend(out : out19[i])
               {
                  dot(&out17[i], &out18[i], &out19[i]);
               }
#pragma omp task firstprivate(i) depend(in : out19[i]) depend(out : out20[i])
               {
                  scale(static_cast<Real>(BATCH_LR), &out19[i], &out20[i]);
               }
#if COMPUTED_ACCUMULATOR_OUTSIDE_KERNEL == 0
#pragma omp task firstprivate(i) depend(inout : hidden_weights_accumulator) depend(in : out20[i])
               {
                  add(hidden_weights_accumulator, &out20[i], hidden_weights_accumulator);
               }
#endif
#if SHOW_LOSS == 1
#pragma omp task firstprivate(i) depend(in : out5[i])
               {
                  const Real loss = sum_square_errors(&out5[i]);
                  if(i % SHOW_LOSS_SAMPLES == 0)
                  {
                     printf("Loss %.5f\n", loss);
                  }
               }
#endif
            }
         }
      }
   }
}
#endif

void network_predict_img(NeuralNetwork* net, Img* img, Matrix* flatten_img, Matrix* out1, Matrix* out2, Matrix* out3,
                         Matrix* out4, Matrix* out5)
{
   matrix_flatten(&img->img_data, 0, flatten_img);
   network_predict(net, flatten_img, out1, out2, out3, out4, out5);
}

void network_predict_img_get_shapes(NeuralNetwork* net, Img* img, Matrix* flatten_img, Pair* shapes)
{
   matrix_flatten(&img->img_data, 0, flatten_img);
   network_predict_get_shapes(net, flatten_img, shapes);
}

void network_predict_imgs_get_shapes(NeuralNetwork* net, Img* img, Matrix* flatten_img, Pair* shapes)
{
   network_predict_img_get_shapes(net, img, flatten_img, shapes);
}

void check_prediction(Img* img, Matrix* output, bool& correct)
{
   if(matrix_argmax(output) == img->label)
   {
      correct = true;
   }
}

void network_predict_get_shapes(NeuralNetwork* net, Matrix* input_data, Pair* shapes)
{
   Matrix* hidden_inputs = dot(net->hidden_weights, input_data);
   shapes[0] = {hidden_inputs->rows, hidden_inputs->cols};
   Matrix* hidden_outputs = apply(sigmoid, hidden_inputs);
   shapes[1] = {hidden_outputs->rows, hidden_outputs->cols};
   Matrix* final_inputs = dot(net->output_weights, hidden_outputs);
   shapes[2] = {final_inputs->rows, final_inputs->cols};
   Matrix* final_outputs = apply(sigmoid, final_inputs);
   shapes[3] = {final_outputs->rows, final_outputs->cols};
   Matrix* res = softmax(final_outputs);
   shapes[4] = {res->rows, res->cols};

   free(hidden_inputs);
   free(hidden_outputs);
   free(final_inputs);
   free(final_outputs);
   free(res);
}

void network_predict(NeuralNetwork* net, Matrix* input_data, Matrix* out1, Matrix* out2, Matrix* out3, Matrix* out4,
                     Matrix* out5)
{
   dot(net->hidden_weights, input_data, out1);
   apply(sigmoid, out1, out2);
   dot(net->output_weights, out2, out3);
   apply(sigmoid, out3, out4);
   softmax(out4, out5);
}

void network_save(NeuralNetwork* net, char const* file_string)
{
   mkdir(file_string, 0777);
   // Write the descriptor file
   chdir(file_string);
   FILE* descriptor = fopen("descriptor", "w");
   fprintf(descriptor, "%d\n", net->input);
   fprintf(descriptor, "%d\n", net->hidden);
   fprintf(descriptor, "%d\n", net->output);
   fclose(descriptor);
   matrix_save(net->hidden_weights, (char const*)"hidden");
   matrix_save(net->output_weights, (char const*)"output");
   fprintf(stderr, "Successfully written to '%s'\n", file_string);
   chdir("-"); // Go back to the orignal directory
}

#if FAKE_DATASET ==  0
NeuralNetwork* network_load(char const* file_string)
{
   NeuralNetwork* net = (NeuralNetwork*)malloc(sizeof(NeuralNetwork));
   char entry[MAXCHAR];
   chdir(file_string);

   FILE* descriptor = fopen("descriptor", "r");
   fgets(entry, MAXCHAR, descriptor);
   net->input = atoi(entry);
   fgets(entry, MAXCHAR, descriptor);
   net->hidden = atoi(entry);
   fgets(entry, MAXCHAR, descriptor);
   net->output = atoi(entry);
   fclose(descriptor);
   net->hidden_weights = matrix_load((char const*)"hidden");
   net->output_weights = matrix_load((char const*)"output");
   fprintf(stderr, "Successfully loaded network from '%s'\n", file_string);
   chdir("-"); // Go back to the original directory
   return net;
}
#else
NeuralNetwork* network_load(char const* file_string)
{
   NeuralNetwork* net = (NeuralNetwork*)malloc(sizeof(NeuralNetwork));
   net->input = IMG_SIZE * IMG_SIZE;
   net->hidden = HIDDEN_LAYER_SIZE;
   net->output = NUM_CLASSES;
   net->hidden_weights = matrix_create(net->hidden, net->input);
   net->output_weights = matrix_create(net->output, net->hidden);
   matrix_randomize(net->hidden_weights, net->hidden);
   matrix_randomize(net->output_weights, net->output);
   return net;
}
#endif

void network_print(NeuralNetwork* net)
{
   fprintf(stderr, "# of Inputs: %d\n", net->input);
   fprintf(stderr, "# of Hidden: %d\n", net->hidden);
   fprintf(stderr, "# of Output: %d\n", net->output);
   fprintf(stderr, "Hidden Weights: \n");
   matrix_print(net->hidden_weights);
   fprintf(stderr, "Output Weights: \n");
   matrix_print(net->output_weights);
}

void img_free(Img* img)
{
   free(img);
   img = nullptr;
}

void imgs_free(Img** imgs, int n)
{
   for(int i = 0; i < n; i++)
   {
      img_free(imgs[i]);
   }
   free(imgs);
   imgs = nullptr;
}

void img_print(Img* img)
{
   matrix_print(&img->img_data);
   fprintf(stderr, "Img Label: %d\n", img->label);
}

#if FAKE_DATASET == 1

void csv_to_imgs(char const* file_string, Img* imgs, int number_of_imgs)
{
   (void)file_string;
   for(int i = 0; i < number_of_imgs; i++)
   {
      imgs[i].label = 1;
      imgs[i].img_data.rows = IMG_SIZE;
      imgs[i].img_data.cols = IMG_SIZE;
      for(unsigned r = 0; r < IMG_SIZE; r++)
      {
         for(unsigned c = 0; c < IMG_SIZE; c++)
         {
            imgs[i].img_data.entries[r * IMG_SIZE + c] = uniform_distribution(0.0, 1.0);
         }
      }
   }
}

#else

void csv_to_imgs(char const* file_string, Img* imgs, int number_of_imgs)
{
#if IMG_SIZE != 28
#error "Image size should be 28"
#endif
   FILE* fp;
   char row[MAXCHAR];
   fp = fopen(file_string, "r");

   // Read the first line
   fgets(row, MAXCHAR, fp);
   int i = 0;
   while(feof(fp) != 1 && i < number_of_imgs)
   {
      int j = 0;
      fgets(row, MAXCHAR, fp);
      char* token = strtok(row, ",");
      while(token != NULL)
      {
         if(j == 0)
         {
            imgs[i].label = atoi(token);
         }
         else
         {
            imgs[i].img_data.rows = IMG_SIZE;
            imgs[i].img_data.cols = IMG_SIZE;
            imgs[i].img_data.entries[((j - 1) / 28) * imgs[i].img_data.cols + ((j - 1) % 28)] = atoi(token) / 256.0;
         }
         token = strtok(NULL, ",");
         j++;
      }
      i++;
   }
   fclose(fp);
}
#endif

#if TRAIN == 0
__attribute__((noinline)) void fn_host(Matrix* in_weights, Matrix* out_weights, Img* imgs, Matrix* flatten_imgs,
                                       bool* correct, Matrix* out1, Matrix* out2, Matrix* out3, Matrix* out4,
                                       Matrix* out5)
{
   for(int i = 0; i < INFERENCE_SAMPLES; i++)
   {
      matrix_flatten(&imgs[i], 0, &flatten_imgs[i]);
      dot(in_weights, &flatten_imgs[i], &out1[i]);
      apply(sigmoid, &out1[i], &out2[i]);
      dot(out_weights, &out2[i], &out3[i]);
      apply(sigmoid, &out3[i], &out4[i]);
      softmax(&out4[i], &out5[i]);
      check_prediction(&imgs[i], &out5[i], correct[i]);
   }
}

#if BANK_NUMBER == 2
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 2 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 2 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 2 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 2 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 2 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 2 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 2 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 2 chunk_size = 1024
#endif
#elif BANK_NUMBER == 32
#ifdef USE_AXI
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 32 chunk_size = 4 mode = m_axi
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 32 chunk_size = 16 mode = m_axi
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 32 chunk_size = 32 mode = m_axi
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 32 chunk_size = 64 mode = m_axi
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 32 chunk_size = 128 mode = m_axi
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 32 chunk_size = 256 mode = m_axi
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 32 chunk_size = 512 mode = m_axi
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 32 chunk_size = 1024 mode = m_axi
#endif
#else
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 32 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 32 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 32 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 32 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 32 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 32 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 32 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 32 chunk_size = 1024
#endif
#endif
#elif BANK_NUMBER == 8
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 8 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 8 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 8 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 8 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 8 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 8 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 8 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 8 chunk_size = 1024
#endif
#elif BANK_NUMBER == 16
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 16 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 16 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 16 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 16 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 16 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 16 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 16 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 16 chunk_size = 1024
#endif
#elif BANK_NUMBER == 4
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 4 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 4 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 4 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 4 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 4 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 4 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 4 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 4 chunk_size = 1024
#endif
#elif BANK_NUMBER == 64
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 64 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 64 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 64 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 64 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 64 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 64 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 64 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 64 chunk_size = 1024
#endif
#elif BANK_NUMBER == 128
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 128 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 128 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 128 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 128 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 128 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 128 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 128 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 128 chunk_size = 1024
#endif
#endif
#ifdef USE_AXI
#ifndef BANK_NUMBER
#pragma HLS bus mode = m_axi
#endif
#pragma HLS interface port = in_weights mode = bus offset = direct
#pragma HLS interface port = out_weights mode = bus offset = direct
#pragma HLS interface port = imgs mode = bus offset = direct
#pragma HLS interface port = flatten_imgs mode = bus offset = direct
#pragma HLS interface port = correct mode = bus offset = direct
#pragma HLS interface port = out1 mode = bus offset = direct
#pragma HLS interface port = out2 mode = bus offset = direct
#pragma HLS interface port = out3 mode = bus offset = direct
#pragma HLS interface port = out4 mode = bus offset = direct
#pragma HLS interface port = out5 mode = bus offset = direct
#else
#pragma HLS interface port = in_weights mode = bus
#pragma HLS interface port = out_weights mode = bus
#pragma HLS interface port = imgs mode = bus
#pragma HLS interface port = flatten_imgs mode = bus
#pragma HLS interface port = correct mode = bus
#pragma HLS interface port = out1 mode = bus
#pragma HLS interface port = out2 mode = bus
#pragma HLS interface port = out3 mode = bus
#pragma HLS interface port = out4 mode = bus
#pragma HLS interface port = out5 mode = bus
#endif
__attribute__((noinline)) void fn(Matrix* in_weights, Matrix* out_weights, Img* imgs, Matrix* flatten_imgs,
                                  bool* correct, Matrix* out1, Matrix* out2, Matrix* out3, Matrix* out4, Matrix* out5)
{
#pragma omp parallel default(none) \
    shared(in_weights, out_weights, imgs, flatten_imgs, correct, out1, out2, out3, out4, out5) num_threads(1)
   {
#pragma omp single nowait
      {
         if(in_weights && out_weights && imgs && flatten_imgs && correct && out1 && out2 && out3 && out4)
         {
            for(int i = 0; i < INFERENCE_SAMPLES; i++)
            {
#pragma omp task firstprivate(i) depend(inout : imgs[i], flatten_imgs[i])
               {
                  matrix_flatten(&imgs[i], 0, &flatten_imgs[i]);
               }
#pragma omp task firstprivate(i) depend(in : in_weights, flatten_imgs[i]) depend(inout : out1[i])
               {
                  dot(in_weights, &flatten_imgs[i], &out1[i]);
               }
#pragma omp task firstprivate(i) depend(in : out1[i]) depend(inout : out2[i])
               {
                  apply(sigmoid, &out1[i], &out2[i]);
               }
#pragma omp task firstprivate(i) depend(in : out_weights, out2[i]) depend(inout : out3[i])
               {
                  dot(out_weights, &out2[i], &out3[i]);
               }
#pragma omp task firstprivate(i) depend(in : out3[i]) depend(inout : out4[i])
               {
                  apply(sigmoid, &out3[i], &out4[i]);
               }
#pragma omp task firstprivate(i) depend(in : out4[i]) depend(inout : out5[i])
               {
                  softmax(&out4[i], &out5[i]);
               }
#pragma omp task firstprivate(i) depend(in : imgs[i], out5[i]) depend(inout : correct[i])
               {
                  check_prediction(&imgs[i], &out5[i], correct[i]);
               }
            }
         }
      }
   }
}
#endif
