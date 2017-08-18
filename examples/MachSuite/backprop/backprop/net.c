#include "net.h"

//Activation function..
TYPE sigmoid(TYPE x){
    TYPE temp, one;    
    one = (double)1.0;
    temp = one + exp(-x);    
    temp = one/temp;
    return temp;
}

void update_layer(int numIn, int numOut, TYPE weightsIn[MAX_ROWS][MAX_COLS], TYPE activationsIn[], TYPE activationsOut[]){
    int ii, jj;
    TYPE sum;
    sum = 0.0;

    ul_1 : for(ii = 0; ii < numOut; ii++){
        sum = 0.0;
        ul_2 : for(jj = 0; jj < numIn; jj++){
            sum = sum + activationsIn[jj] * weightsIn[jj][ii];
        }
        activationsOut[ii] = sigmoid(sum);
    }
}

void update(TYPE weights[][MAX_ROWS][MAX_COLS], 
        TYPE *input,
        TYPE activations[][MAX_ROWS]){
    int i;

    //initialize first activation layer to be the inputs to the nnet..
    u_1 : for(i = 0; i < layer_size[0]; i++){
        activations[0][i] = input[i];
    }

    //iterate over each weight layer...
    u_2 : for(i = 0; i < NUM_LAYERS - 1; i++){
        update_layer(layer_size[i], layer_size[i + 1], weights[i], activations[i], activations[i + 1]);
    }
}

void propagate_error_out(TYPE *activationsOut, TYPE *deltas, TYPE *targets){
    int i;
    TYPE error;
    error = 0.0;

    //from the output layer to the N-1 hidden layer update the deltas...
    p_1 : for(i = 0; i < layer_size[NUM_LAYERS - 1]; i++){
        error = targets[i] - activationsOut[i];
        deltas[i] = sigmoid(activationsOut[i]) * error;
    }
}


void propagate_error_layer(int level, 
        int level_in, int level_out,
        TYPE weightsIn[MAX_ROWS][MAX_COLS], 
        TYPE *activationsOut, 
        TYPE *deltasIn, TYPE *deltasOut){

    int kk, jj;
    TYPE error;

    //for the take in the last calc activities and deltas and comp differences...
    //In and Out are relative to the way BACKPROP is moving... opposite the forward pass... 

    pel_1 : for(jj = 0; jj < layer_size[level]; jj++){
        error = 0.0;
        pel_2 : for(kk = 0; kk < layer_size[level + 1]; kk++){
            error = error + deltasIn[kk] * weightsIn[jj][kk];
        }
        deltasOut[jj] = sigmoid(activationsOut[jj]) * error;
    }
}

void update_weights(int level,
        TYPE weights[][MAX_COLS], 
        TYPE *deltas, TYPE activations[], 
        TYPE changeMat[][MAX_COLS]){

    int ii, jj;
    TYPE change, newW;

    uw_1 : for(ii = 0; ii < layer_size[level - 1]; ii++){
        uw_2 : for(jj = 0; jj < layer_size[level]; jj++){
            change = deltas[jj]*activations[ii];
            newW = weights[ii][jj] + N*change + M*changeMat[ii][jj];

            //Can try without using a change matrix.. saves space! lowers accuracy...
            //newW = weights[ii][jj] + N*change;// + M*changeMat[ii][jj];
            weights[ii][jj] = newW;
            changeMat[ii][jj] = change;
        }
    }
}

void propagate_errors(TYPE weights[][MAX_ROWS][MAX_COLS], 
        TYPE activations[][MAX_ROWS], 
        TYPE *targets, 
        TYPE changeMat[NUM_LAYERS - 1][MAX_ROWS][MAX_COLS]){

    int i, j;
    TYPE deltas[NUM_LAYERS][MAX_ROWS];
    //TYPE deltas[NUM_LAYERS - 1][MAX_ROWS];

    pe_1 : for(i = 0; i < NUM_LAYERS - 1; i++){
        pe_2 : for(j = 0; j<MAX_ROWS; j++){
            deltas[i][j] = 0.0;
        }
    }

    propagate_error_out(activations[NUM_LAYERS - 1], deltas[NUM_LAYERS - 2], targets);

    pe_3 : for(i = NUM_LAYERS - 2; i > 0; i--){
        j = i - 1;
        propagate_error_layer(i, j, i, weights[i], activations[i], deltas[i], deltas[j]);
    }

    pe_4 : for(i = NUM_LAYERS - 1; i >= 1; i--){
        j = i - 1;
        update_weights(i, weights[j], deltas[j], activations[j], changeMat[i]);
    }
}

TYPE comp_error(TYPE *targets, TYPE *activations){
    int i;
    TYPE error, temp;

    error = 0.0;
    temp = 0.0;

    ce_1 :for(i = 0; i < SIZE_OUT; i++){
        temp = targets[i] - activations[i];
        error = error + 0.5*temp*temp;
    }

    return error;
}

void backprop(TYPE weights[NUM_LAYERS - 1][MAX_ROWS][MAX_COLS], 
        TYPE inputs[NUM_TRAIN][SIZE_IN], 
        TYPE targets[NUM_TRAIN][SIZE_OUT]){

    int ee, rows, cols, jj;
    TYPE error;
    error = 0.0;

    TYPE changeMat[NUM_LAYERS][MAX_ROWS][MAX_COLS];
    TYPE activations[NUM_LAYERS][MAX_ROWS];

    bp_1 : for(ee = 0; ee < NUM_LAYERS; ee++){
        bp_2 : for(rows = 0; rows < MAX_ROWS; rows++){
            activations[ee][rows]= 1.0;
            bp_3 : for(cols = 0; cols < MAX_COLS; cols++){
                changeMat[ee][rows][cols] = 0.0;
            }
        }
    }

    epochs_l : for(ee = 0; ee < EPOCS; ee++){
        error = 0.0;
        init_1 : for(jj = 0; jj < NUM_TRAIN; jj++){
            init_2 : for(cols = 0; cols < NUM_LAYERS; cols++){
                init_3 : for(rows = 0; rows < MAX_ROWS; rows++){
                    activations[cols][rows]= 1.0;
                }
            }

            //Run forward pass of the training data through the network
            //get activations input stimulated
            update(weights, inputs[jj], activations);
            
            //Adjust weights based on deltas between generated output and
            //known answer
            propagate_errors(weights, activations, targets[jj], changeMat);

            //Currently using synthetic I/O
            //error rate is irrelivant
            error += comp_error(targets[jj], activations[NUM_LAYERS - 1]);
        }
    }
}

