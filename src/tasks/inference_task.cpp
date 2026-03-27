#include <Arduino.h>
#include "tasks/inference_task.h"
#include "shared.h"
#include "weights.h"

/* fully connected forward */
static void fc(
    const float *in,
    int in_dim,
    const float *W,
    const float *b,
    float *out,
    int out_dim
) {
    for (int i = 0; i < out_dim; i++) {
        float acc = b[i];
        const float *row = W + i * in_dim;
        /* dot product between input and current ith weights row */
        for (int j = 0; j < in_dim; j++)
            acc += row[j] * in[j];
        out[i] = acc;
    }
}

/* fused fullyconnected + relu */
static void fc_relu(
    const float *in,
    int in_dim,
    const float *W,
    const float *b,
    float *out,
    int out_dim
) {
    for (int i = 0; i < out_dim; i++) {
        float acc = b[i];
        const float *row = W + i * in_dim;
        for (int j = 0; j < in_dim; j++)
            acc += row[j] * in[j];
        out[i] = acc > 0.0f ? acc : 0.0f;   /* ReLU */
    }
}

/* forward autoencoder */
static void run_inference(const float input[IN_DIM], float output[DEC1_DIM]) {
    /* to avoid stack allocation each time we use static lifetimes here */
    static float h1[ENC0_DIM];
    static float h2[ENC1_DIM];
    static float h3[DEC1_DIM];

    fc_relu(input, IN_DIM, encoder_0_weight, encoder_0_bias, h1, ENC0_DIM);
    fc_relu(h1, ENC0_DIM, encoder_1_weight, encoder_1_bias, h2, ENC1_DIM);
    fc_relu(h2, ENC1_DIM, decoder_0_weight, decoder_0_bias, h3, DEC0_DIM);
    fc(h3, DEC0_DIM, decoder_1_weight, decoder_1_bias, output, DEC1_DIM);
}

/* run inference each IN_DIM samples */
void inference_task(void *params) {
    float window[IN_DIM];
    float model_output[DEC1_DIM];
    int samples_collected = 0;

    for (;;) {
        SensorData sample;
        xQueueReceive(sensor_queue, &sample, portMAX_DELAY);

        /* pack into the flat window buffer */
        window[samples_collected * 2] = sample.acc_magnitude;
        window[samples_collected * 2 + 1] = sample.rot_magnitude;
        samples_collected++;

        /* don't do inference yet */
        if (samples_collected < 20)
            continue;

        /* we have a full window to run the network */
        run_inference(window, model_output);

        /* mean squared error between input/output */
        float mse = 0.0f;
        for (int i = 0; i < IN_DIM; i++) {
            float diff = window[i] - model_output[i];
            mse += diff * diff;
        }
        mse = mse / IN_DIM;

        xQueueSend(inference_queue, &mse, 0);

        /* reset for the next inference chunk */
        samples_collected = 0;
    }
}
