#!/usr/bin/env python3
"""
train_model.py — Phase 9

Trains a Soft Voting Ensemble (Random Forest + SVM + XGBoost) on the generated
deadlock feature dataset to predict whether a deadlock is imminent.
Saves the trained pipeline for live scoring.
"""

import os
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.ensemble import RandomForestClassifier, VotingClassifier
from sklearn.svm import SVC
from xgboost import XGBClassifier
from sklearn.metrics import classification_report, confusion_matrix
from sklearn.pipeline import Pipeline
import joblib

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DATA_PATH = os.path.join(ROOT_DIR, 'predictor', 'data', 'training_data.csv')
MODEL_OUT = os.path.join(ROOT_DIR, 'predictor', 'models', 'deadlock_predictor.pkl')

def train():
    if not os.path.exists(DATA_PATH):
        print(f"Dataset not found at {DATA_PATH}. Run simulate_runs.py first.")
        return

    print("Loading dataset...")
    df = pd.read_csv(DATA_PATH)
    
    if len(df) == 0:
        print("Dataset is empty.")
        return

    # Features and Target
    X = df[['blocked_count', 'wait_time_growth', 'edge_count', 'graph_density']]
    y = df['is_deadlock_imminent']

    print(f"Total samples: {len(df)}")
    print(f"Class balance:\n{y.value_counts()}")

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42, stratify=y)

    print("\nInitializing base models...")
    # Base Model 1: Random Forest
    rf = RandomForestClassifier(n_estimators=100, max_depth=5, class_weight="balanced", random_state=42)
    
    # Base Model 2: SVM (must have probability=True for soft voting)
    svm = SVC(kernel='rbf', probability=True, class_weight='balanced', random_state=42)
    
    # Base Model 3: XGBoost
    xgb = XGBClassifier(n_estimators=100, max_depth=4, learning_rate=0.1, 
                        scale_pos_weight=(len(y)-y.sum())/y.sum() if y.sum()>0 else 1, 
                        random_state=42, use_label_encoder=False, eval_metric='logloss')

    print("Building Soft Voting Ensemble...")
    # Soft Voting Ensemble blends the probability predictions of the base models
    ensemble = VotingClassifier(
        estimators=[
            ('rf', rf),
            ('svm', svm),
            ('xgb', xgb)
        ],
        voting='soft'
    )

    # Wrap in a pipeline with a standard scaler
    pipeline = Pipeline([
        ('scaler', StandardScaler()),
        ('ensemble', ensemble)
    ])

    print("Training pipeline...")
    pipeline.fit(X_train, y_train)

    print("\nEvaluating model on test set...")
    y_pred = pipeline.predict(X_test)
    print("Confusion Matrix:")
    print(confusion_matrix(y_test, y_pred))
    print("\nClassification Report:")
    print(classification_report(y_test, y_pred))

    os.makedirs(os.path.dirname(MODEL_OUT), exist_ok=True)
    joblib.dump(pipeline, MODEL_OUT)
    print(f"\nModel saved successfully to {MODEL_OUT}")

if __name__ == "__main__":
    train()
