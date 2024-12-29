#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

// Constants
#define MAX_LAYERS 10
#define IMAGE_HEIGHT 256
#define IMAGE_WIDTH 256
#define IMAGE_CHANNELS 3
#define BATCH_SIZE 16
#define EPSILON 1e-7

// Structures for different layer types
typedef struct {
    float* weights;
    float* biases;
    float* output;
    int filters;
    int kernel_size;
    int input_channels;
    int input_height;
    int input_width;
} Conv2DLayer;

typedef struct {
    int pool_size;
    float* output;
} MaxPool2DLayer;

typedef struct {
    float* weights;
    float* biases;
    float* output;
    int units;
    int input_dim;
} DenseLayer;

typedef struct {
    float* output;
} FlattenLayer;

typedef enum {
    CONV2D,
    MAXPOOL2D,
    FLATTEN,
    DENSE
} LayerType;

typedef struct {
    void* layer;
    LayerType type;
} Layer;

typedef struct {
    Layer layers[MAX_LAYERS];
    int num_layers;
    float learning_rate;
} Sequential;

// Helper functions
float random_float() {
    return ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
}

float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-fminf(fmaxf(x, -500.0f), 500.0f)));
}

float relu(float x) {
    return fmaxf(0.0f, x);
}

// Layer initialization functions
Conv2DLayer* create_conv2d(int filters, int kernel_size, int input_channels, 
                          int input_height, int input_width) {
    Conv2DLayer* layer = (Conv2DLayer*)malloc(sizeof(Conv2DLayer));
    layer->filters = filters;
    layer->kernel_size = kernel_size;
    layer->input_channels = input_channels;
    layer->input_height = input_height;
    layer->input_width = input_width;
    
    int weights_size = filters * kernel_size * kernel_size * input_channels;
    layer->weights = (float*)malloc(weights_size * sizeof(float));
    layer->biases = (float*)malloc(filters * sizeof(float));
    
    // Initialize weights with small random values
    for (int i = 0; i < weights_size; i++) {
        layer->weights[i] = random_float() * 0.1f;
    }
    
    memset(layer->biases, 0, filters * sizeof(float));
    
    int output_height = input_height - kernel_size + 1;
    int output_width = input_width - kernel_size + 1;
    layer->output = (float*)malloc(output_height * output_width * filters * sizeof(float));
    
    return layer;
}

MaxPool2DLayer* create_maxpool2d(int pool_size) {
    MaxPool2DLayer* layer = (MaxPool2DLayer*)malloc(sizeof(MaxPool2DLayer));
    layer->pool_size = pool_size;
    return layer;
}

DenseLayer* create_dense(int units, int input_dim) {
    DenseLayer* layer = (DenseLayer*)malloc(sizeof(DenseLayer));
    layer->units = units;
    layer->input_dim = input_dim;
    
    layer->weights = (float*)malloc(input_dim * units * sizeof(float));
    layer->biases = (float*)malloc(units * sizeof(float));
    
    // He initialization
    float scale = sqrtf(2.0f / input_dim);
    for (int i = 0; i < input_dim * units; i++) {
        layer->weights[i] = random_float() * scale;
    }
    
    memset(layer->biases, 0, units * sizeof(float));
    layer->output = (float*)malloc(units * sizeof(float));
    
    return layer;
}

FlattenLayer* create_flatten() {
    return (FlattenLayer*)malloc(sizeof(FlattenLayer));
}

// Forward pass functions
void conv2d_forward(Conv2DLayer* layer, float* input) {
    int out_height = layer->input_height - layer->kernel_size + 1;
    int out_width = layer->input_width - layer->kernel_size + 1;
    
    // For each filter
    for (int f = 0; f < layer->filters; f++) {
        // For each output position
        for (int i = 0; i < out_height; i++) {
            for (int j = 0; j < out_width; j++) {
                float sum = layer->biases[f];
                
                // For each kernel position
                for (int ki = 0; ki < layer->kernel_size; ki++) {
                    for (int kj = 0; kj < layer->kernel_size; kj++) {
                        for (int c = 0; c < layer->input_channels; c++) {
                            int input_idx = ((i + ki) * layer->input_width + (j + kj)) * 
                                          layer->input_channels + c;
                            int weight_idx = ((f * layer->kernel_size + ki) * 
                                           layer->kernel_size + kj) * 
                                           layer->input_channels + c;
                            sum += input[input_idx] * layer->weights[weight_idx];
                        }
                    }
                }
                
                layer->output[(i * out_width + j) * layer->filters + f] = sum;
            }
        }
    }
}

void maxpool2d_forward(MaxPool2DLayer* layer, float* input, int height, int width, int channels) {
    int out_height = height / layer->pool_size;
    int out_width = width / layer->pool_size;
    
    for (int c = 0; c < channels; c++) {
        for (int i = 0; i < out_height; i++) {
            for (int j = 0; j < out_width; j++) {
                float max_val = -INFINITY;
                
                // Find maximum in pool window
                for (int pi = 0; pi < layer->pool_size; pi++) {
                    for (int pj = 0; pj < layer->pool_size; pj++) {
                        int input_idx = ((i * layer->pool_size + pi) * width + 
                                       (j * layer->pool_size + pj)) * channels + c;
                        max_val = fmaxf(max_val, input[input_idx]);
                    }
                }
                
                layer->output[(i * out_width + j) * channels + c] = max_val;
            }
        }
    }
}

void dense_forward(DenseLayer* layer, float* input) {
    for (int i = 0; i < layer->units; i++) {
        float sum = layer->biases[i];
        for (int j = 0; j < layer->input_dim; j++) {
            sum += input[j] * layer->weights[j * layer->units + i];
        }
        layer->output[i] = sigmoid(sum);
    }
}

// Sequential model functions
Sequential* create_sequential() {
    Sequential* model = (Sequential*)malloc(sizeof(Sequential));
    model->num_layers = 0;
    model->learning_rate = 0.01f;
    return model;
}

void add_layer(Sequential* model, void* layer, LayerType type) {
    if (model->num_layers < MAX_LAYERS) {
        model->layers[model->num_layers].layer = layer;
        model->layers[model->num_layers].type = type;
        model->num_layers++;
    }
}

float* forward_pass(Sequential* model, float* input) {
    float* current_input = input;
    
    for (int i = 0; i < model->num_layers; i++) {
        Layer layer = model->layers[i];
        
        switch (layer.type) {
            case CONV2D:
                conv2d_forward((Conv2DLayer*)layer.layer, current_input);
                current_input = ((Conv2DLayer*)layer.layer)->output;
                break;
                
            case MAXPOOL2D:
                // Note: Need to pass proper dimensions
                maxpool2d_forward((MaxPool2DLayer*)layer.layer, current_input, 
                                IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_CHANNELS);
                current_input = ((MaxPool2DLayer*)layer.layer)->output;
                break;
                
            case DENSE:
                dense_forward((DenseLayer*)layer.layer, current_input);
                current_input = ((DenseLayer*)layer.layer)->output;
                break;
                
            case FLATTEN:
                // Flattening is implicit in the dense layer input
                break;
        }
    }
    
    return current_input;
}

// Main function showing usage
int main() {
    srand(time(NULL));
    
    // Create model
    Sequential* model = create_sequential();
    
    // Add layers
    add_layer(model, create_conv2d(32, 2, 3, IMAGE_HEIGHT, IMAGE_WIDTH), CONV2D);
    add_layer(model, create_maxpool2d(2), MAXPOOL2D);
    add_layer(model, create_conv2d(64, 2, 32, IMAGE_HEIGHT/2, IMAGE_WIDTH/2), CONV2D);
    add_layer(model, create_maxpool2d(2), MAXPOOL2D);
    add_layer(model, create_flatten(), FLATTEN);
    add_layer(model, create_dense(64, IMAGE_HEIGHT*IMAGE_WIDTH*64), DENSE);
    add_layer(model, create_dense(1, 64), DENSE);
    
    // Create dummy input
    float* input = (float*)malloc(IMAGE_HEIGHT * IMAGE_WIDTH * IMAGE_CHANNELS * sizeof(float));
    for (int i = 0; i < IMAGE_HEIGHT * IMAGE_WIDTH * IMAGE_CHANNELS; i++) {
        input[i] = random_float();
    }
    
    // Forward pass
    float* output = forward_pass(model, input);
    
    // Print first output
    printf("Output: %f\n", output[0]);
    
    // Free memory
    free(input);
    // TODO: Add proper cleanup for all layers
    
    return 0;
}
