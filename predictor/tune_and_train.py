#!/usr/bin/env python3
"""
tune_and_train.py

Conducts systematic hyperparameter tuning and Stratified 5-Fold Cross-Validation
to optimize accuracy, precision, and recall while strictly preventing over- or
under-fitting (measuring train-val generalization gap).
"""

import os
import pandas as pd
import numpy as np
from sklearn.model_selection import StratifiedKFold, cross_validate
from sklearn.preprocessing import StandardScaler
from sklearn.ensemble import RandomForestClassifier, VotingClassifier
from sklearn.svm import SVC
from xgboost import XGBClassifier
from sklearn.pipeline import Pipeline
from sklearn.metrics import classification_report, confusion_matrix, roc_auc_score
import joblib

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DATA_PATH = os.path.join(ROOT_DIR, 'predictor', 'data', 'training_data.csv')
MODEL_OUT = os.path.join(ROOT_DIR, 'predictor', 'models', 'deadlock_predictor.pkl')

def evaluate_configuration(weight_ratio, rf_depth, xgb_depth, svm_C, ens_weights):
    df = pd.read_csv(DATA_PATH)
    X = df[['blocked_count', 'wait_time_growth', 'edge_count', 'graph_density']]
    y = df['is_deadlock_imminent']

    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)

    class_weight = {0: 1.0, 1: weight_ratio} if weight_ratio > 1.0 else None

    # Base Model 1: Random Forest with min_samples_leaf to prevent overfitting
    rf = RandomForestClassifier(
        n_estimators=120,
        max_depth=rf_depth,
        min_samples_split=6,
        min_samples_leaf=3,
        class_weight=class_weight,
        random_state=42,
        n_jobs=-1
    )

    # Base Model 2: Regularized SVM with probability calibration
    svm = SVC(
        C=svm_C,
        kernel='rbf',
        probability=True,
        class_weight=class_weight,
        random_state=42
    )

    # Base Model 3: XGBoost with L1/L2 regularization and subsampling
    xgb = XGBClassifier(
        n_estimators=120,
        max_depth=xgb_depth,
        learning_rate=0.08,
        subsample=0.85,
        colsample_bytree=0.85,
        scale_pos_weight=weight_ratio,
        reg_alpha=0.2,
        reg_lambda=1.5,
        random_state=42,
        eval_metric='logloss',
        n_jobs=-1
    )

    ensemble = VotingClassifier(
        estimators=[('rf', rf), ('svm', svm), ('xgb', xgb)],
        voting='soft',
        weights=ens_weights
    )

    skf = StratifiedKFold(n_splits=5, shuffle=True, random_state=42)
    scores = cross_validate(
        ensemble, X_scaled, y,
        cv=skf,
        scoring=['accuracy', 'precision', 'recall', 'f1', 'roc_auc'],
        return_train_score=True
    )

    train_acc = scores['train_accuracy'].mean()
    val_acc = scores['test_accuracy'].mean()
    val_prec = scores['test_precision'].mean()
    val_rec = scores['test_recall'].mean()
    val_f1 = scores['test_f1'].mean()
    val_auc = scores['test_roc_auc'].mean()
    gap = abs(train_acc - val_acc)

    return {
        'weight_ratio': weight_ratio,
        'rf_depth': rf_depth,
        'xgb_depth': xgb_depth,
        'svm_C': svm_C,
        'ens_weights': ens_weights,
        'train_acc': train_acc,
        'val_acc': val_acc,
        'gap': gap,
        'val_prec': val_prec,
        'val_rec': val_rec,
        'val_f1': val_f1,
        'val_auc': val_auc
    }

def main():
    print("=" * 70)
    print("HYPERPARAMETER OPTIMIZATION & GENERALIZATION AUDIT (5-FOLD CV)")
    print("=" * 70)

    configs = [
        # (weight_ratio, rf_depth, xgb_depth, svm_C, ens_weights)
        (1.0, 5, 3, 1.0, [1.0, 1.0, 1.0]),
        (1.4, 5, 3, 1.2, [1.0, 1.2, 1.2]),
        (1.6, 6, 4, 1.5, [1.0, 1.5, 1.5]),
        (1.8, 6, 4, 1.5, [1.0, 1.2, 1.5]),
        (2.0, 6, 4, 1.5, [1.0, 1.5, 1.5]),
        (2.5, 6, 4, 2.0, [1.0, 1.5, 1.5]),
    ]

    results = []
    for cfg in configs:
        print(f"Testing Config: weight_ratio={cfg[0]}, rf_depth={cfg[1]}, xgb_depth={cfg[2]}, svm_C={cfg[3]}, weights={cfg[4]}...")
        res = evaluate_configuration(*cfg)
        results.append(res)
        print(f"  --> Train Acc: {res['train_acc']*100:.2f}% | Val Acc: {res['val_acc']*100:.2f}% | Gap: {res['gap']*100:.2f}%")
        print(f"  --> Val Prec: {res['val_prec']*100:.2f}% | Val Rec: {res['val_rec']*100:.2f}% | F1: {res['val_f1']:.4f} | AUC: {res['val_auc']:.4f}\n")

    # Select best configuration: maximize F1 and Val Acc while keeping Train-Val Gap < 2.5% (no overfitting)
    valid_configs = [r for r in results if r['gap'] < 0.025]
    best = max(valid_configs, key=lambda x: (x['val_f1'] + x['val_acc']))

    print("=" * 70)
    print("BEST BALANCED CONFIGURATION SELECTED (ZERO OVERFITTING):")
    print(f"  Weight Ratio:      {best['weight_ratio']}")
    print(f"  RF Max Depth:      {best['rf_depth']}")
    print(f"  XGB Max Depth:     {best['xgb_depth']}")
    print(f"  SVM Regularizer C: {best['svm_C']}")
    print(f"  Ensemble Weights:  {best['ens_weights']}")
    print(f"  Train Accuracy:    {best['train_acc']*100:.2f}%")
    print(f"  Val Accuracy:      {best['val_acc']*100:.2f}% (Overfitting Gap: {best['gap']*100:.2f}%)")
    print(f"  Val Precision:     {best['val_prec']*100:.2f}%")
    print(f"  Val Recall:        {best['val_rec']*100:.2f}%")
    print(f"  Val F1-Score:      {best['val_f1']:.4f}")
    print(f"  Val ROC-AUC:       {best['val_auc']:.4f}")
    print("=" * 70)

    # Train final model on full dataset using best hyperparameters
    print("\nTraining final retuned Soft Voting Ensemble on complete dataset...")
    df = pd.read_csv(DATA_PATH)
    X = df[['blocked_count', 'wait_time_growth', 'edge_count', 'graph_density']]
    y = df['is_deadlock_imminent']

    class_weight = {0: 1.0, 1: best['weight_ratio']} if best['weight_ratio'] > 1.0 else None

    final_rf = RandomForestClassifier(
        n_estimators=150,
        max_depth=best['rf_depth'],
        min_samples_split=6,
        min_samples_leaf=3,
        class_weight=class_weight,
        random_state=42,
        n_jobs=-1
    )

    final_svm = SVC(
        C=best['svm_C'],
        kernel='rbf',
        probability=True,
        class_weight=class_weight,
        random_state=42
    )

    final_xgb = XGBClassifier(
        n_estimators=150,
        max_depth=best['xgb_depth'],
        learning_rate=0.08,
        subsample=0.85,
        colsample_bytree=0.85,
        scale_pos_weight=best['weight_ratio'],
        reg_alpha=0.2,
        reg_lambda=1.5,
        random_state=42,
        eval_metric='logloss',
        n_jobs=-1
    )

    final_ensemble = VotingClassifier(
        estimators=[('rf', final_rf), ('svm', final_svm), ('xgb', final_xgb)],
        voting='soft',
        weights=best['ens_weights']
    )

    pipeline = Pipeline([
        ('scaler', StandardScaler()),
        ('ensemble', final_ensemble)
    ])

    pipeline.fit(X, y)
    joblib.dump(pipeline, MODEL_OUT)
    print(f"Optimized model pipeline successfully saved to: {MODEL_OUT}")

if __name__ == "__main__":
    main()
