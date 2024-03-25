import matplotlib.pyplot as plt
import numpy as np
import tensorflow as tf
from keras.models import load_model

# Load data
test_data = np.load('datasets/input_data_test.npy')
test_targets = np.load('datasets/input_targets_test.npy')

# Load model
model = load_model('models/model-20240325-201855')


system_sample_rate = 115
sequence_length = system_sample_rate * 2    # Look back 3 seconds
sequence_stride = 1                         # Shift one sequence_length at a time (rolling window)
sampling_rate = 1                           # Used for downsampling
batch_size = 1024

# Create test dataset
test_dataset = tf.keras.utils.timeseries_dataset_from_array(
    data=test_data,
    targets=test_targets,
    sequence_length=sequence_length,
    sequence_stride=sequence_stride,
    sampling_rate=sampling_rate,
    batch_size=batch_size
)

predictions = model.predict(test_dataset)
print('prediction shape:', predictions.shape)
print('target shape:', test_targets.shape)
# Plot and inspect
# plt.figure(figsize=(10, 6))
# plt.plot(predictions, 'r', label='Predictions')
# plt.plot(test_targets, 'b', label='Targets')
# plt.legend()
# plt.show()
