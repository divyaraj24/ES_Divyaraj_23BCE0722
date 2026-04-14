# Training Assets

This folder contains the TinyML impact model training artifacts used for submission.

## Files

- `generate_synthetic_dataset.py`: creates synthetic training data using class rules from the training documentation.
- `synthetic_training_data.csv`: generated dataset (4,000 samples; 1,000 per class).
- `train_tinyml_model.py`: training script for the 6->10->4 MLP model.

## Dataset Schema

- magnitude
- jerk
- variance
- peak_freq
- zero_cross
- duration
- class_id
- class_label

## Usage

1. Generate data:
   `python generate_synthetic_dataset.py`
2. Train model:
   `python train_tinyml_model.py`
