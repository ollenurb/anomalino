import os

import numpy as np
import pandas as pd
import torch
import torch.nn as nn
from torch.utils.data import DataLoader, Dataset

# ── Config ────────────────────────────────────────────────────────────────────
CSV_DIR = "/content/anomaly_detection"  # folder containing your normal CSVs
WINDOW = 20  # samples per window
BATCH_SIZE = 32
EPOCHS = 200
LR = 1e-3
THRESHOLD = None  # set manually after inspecting train errors, or use auto below


def smooth(df, window=5):
    df = df.copy()
    for col in list(df.columns.values):
        df[col] = df[col].rolling(window, center=True, min_periods=1).mean()
    return df


# defines a base data pipeline
def load_data(csv_dir, filename):
    df = pd.read_csv(os.path.join(csv_dir, filename))
    # df = despike(df)
    df = smooth(df)
    return df


# ── Features ──────────────────────────────────────────────────────────────────
def extract_features(df):
    df = df.copy()
    df["accel_mag"] = np.sqrt(df["x"] ** 2 + df["y"] ** 2 + df["z"] ** 2)
    df["rot_mag"] = np.sqrt(df["roll"] ** 2 + df["pitch"] ** 2 + df["yaw"] ** 2)
    return df[["accel_mag", "rot_mag"]].values.astype(np.float32)


class SensorDataset(Dataset):
    def __init__(self, csv_dir, window=WINDOW, device="cuda"):
        files = [f for f in os.listdir(csv_dir) if f.endswith(".csv")]
        assert files, f"No CSV files found in {csv_dir}"

        # load files, extract features
        self.all_feats = []
        for f in files:
            df = load_data(csv_dir, f)
            self.all_feats.append(extract_features(df))

        # compute global mean/std
        all_feats_array = np.concatenate(self.all_feats, axis=0)
        self.mean = all_feats_array.mean(axis=0)
        self.std = all_feats_array.std(axis=0) + 1e-9

        self.index = []
        # normalize features, the put them into a tensor -> device
        for feat_idx, feat in enumerate(self.all_feats):
            feat = (feat - self.mean) / self.std
            self.all_feats[feat_idx] = torch.tensor(feat, device=device)
            # precompute indexes
            for i in range(len(feat) - window + 1):
                self.index.append((feat_idx, i, i + window))

    def __len__(self):
        return len(self.index)

    def __getitem__(self, i):
        feat_idx, start, end = self.index[i]
        return self.all_feats[feat_idx][start:end]


# Fully connected autoencoder
class Autoencoder(nn.Module):
    def __init__(self, n_features=2, seq_len=20):
        super().__init__()
        flat = n_features * seq_len
        self.encoder = nn.Sequential(
            nn.Linear(flat, 16),
            nn.ReLU(),
            nn.Linear(16, 4),
            nn.ReLU(),
        )
        self.decoder = nn.Sequential(
            nn.Linear(4, 16),
            nn.ReLU(),
            nn.Linear(16, flat),
        )

    def forward(self, x):
        b = x.size(0)
        # reshape to batch * n_features (basically flatten the tensor)
        z = self.encoder(x.reshape(b, -1))
        return self.decoder(z).reshape(b, WINDOW, -1)


# Convolutional Autoencoder
class ConvAutoencoder(nn.Module):
    def __init__(self, n_features=2, seq_len=WINDOW, kernel_size=7):
        super().__init__()
        self.seq_len = seq_len
        self.n_features = n_features
        pad = (kernel_size - 1) // 2  # =3 for k=7, =1 for k=3

        self.encoder = nn.Sequential(
            nn.Conv1d(n_features, 8, kernel_size=kernel_size, padding=pad),
            nn.ReLU(),
            nn.Conv1d(8, 4, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.AdaptiveAvgPool1d(1),
            nn.Flatten(),
        )

        self.decoder = nn.Sequential(
            nn.Linear(4, 4 * seq_len),
            nn.ReLU(),
            nn.Unflatten(1, (4, seq_len)),
            nn.ConvTranspose1d(4, 8, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.ConvTranspose1d(8, n_features, kernel_size=kernel_size, padding=pad),
        )

    def forward(self, x):
        x = x.permute(0, 2, 1)
        z = self.encoder(x)
        out = self.decoder(z)
        return out.permute(0, 2, 1)


# Train
def validate(model, val_loader, criterion, device):
    model.eval()
    total = 0
    with torch.no_grad():
        for batch in val_loader:
            batch = batch.to(device)
            total += criterion(model(batch), batch).item()
    return total / len(val_loader)


def train(
    model, train_loader, val_loader, epochs=EPOCHS, lr=LR, device="cuda", patience=20
):
    opt = torch.optim.Adam(model.parameters(), lr=lr)
    scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(opt, patience=10, factor=0.5)
    criterion = nn.MSELoss()
    model.to(device)

    best_val_loss = float("inf")
    best_state = None
    epochs_no_improve = 0
    train_losses = []

    for epoch in range(1, epochs + 1):
        model.train()
        train_total = 0
        for batch in train_loader:
            batch = batch.to(device)
            opt.zero_grad()
            loss = criterion(model(batch), batch)
            loss.backward()
            opt.step()
            train_total += loss.item()
        train_loss = train_total / len(train_loader)
        scheduler.step(train_loss)

        train_losses.append(train_loss)

        if epoch % 10 == 0:
            val_loss = validate(model, val_loader, criterion, device)
            print(
                f"Epoch {epoch:>3}/{epochs}  train={train_loss:.6f}  val={val_loss:.6f}  lr={opt.param_groups[0]['lr']:.1e}"
            )
            if val_loss < best_val_loss:
                best_val_loss = val_loss
                best_state = model.state_dict().copy()
                epochs_no_improve = 0
            else:
                epochs_no_improve += 1
                if epochs_no_improve >= patience:
                    print(
                        f"\nEarly stopping at epoch {epoch}  best_val={best_val_loss:.6f}"
                    )
                    break

    model.load_state_dict(best_state)
    return model, train_losses


# Test
def test(model, csv_dir, csv_filename, train_mean, train_std, threshold=None):
    df = load_data(csv_dir, csv_filename)
    feat = extract_features(df)
    feat_norm = (feat - train_mean) / train_std

    errors, timestamps = [], []
    t = df["timestamp"].values
    model.eval()
    with torch.no_grad():
        for i in range(len(feat_norm) - WINDOW + 1):
            window = torch.tensor(feat_norm[i : i + WINDOW]).unsqueeze(0)
            rec = model(window)
            err = nn.MSELoss()(rec, window).item()
            errors.append(err)
            timestamps.append(t[i + WINDOW // 2] - t[0])

    errors = np.array(errors)
    thresh = threshold or (errors.mean() + 3 * errors.std())
    anomalies = np.where(errors > thresh)[0]

    print(f"\nTest file : {csv_filename}")
    print(f"Threshold : {thresh:.6f}")
    print(f"Anomalies : {len(anomalies)} windows")

    return df, feat, timestamps, errors, thresh


def to_c_array(tensor, name, dtype="float"):
    data = tensor.detach().cpu().numpy().flatten()
    vals = ", ".join(f"{v:.6f}f" for v in data)
    shape = " x ".join(str(s) for s in tensor.shape)
    return f"// shape: {shape}\nconst {dtype} {name}[] = {{{vals}}};\n"


if __name__ == "__main__":
    device = "cpu"

    from torch.utils.data import random_split

    dataset = SensorDataset(CSV_DIR, device=device)
    val_size = int(0.2 * len(dataset))
    train_size = len(dataset) - val_size
    train_set, val_set = random_split(dataset, [train_size, val_size])

    train_loader = DataLoader(train_set, batch_size=BATCH_SIZE, shuffle=True)
    val_loader = DataLoader(val_set, batch_size=BATCH_SIZE, shuffle=False)
    train_loader = DataLoader(
        dataset, batch_size=BATCH_SIZE, shuffle=True, pin_memory=False
    )

    print(f"Dataset: {len(dataset)} windows from {CSV_DIR}")

    model = Autoencoder(n_features=2)
    # model = ConvAutoencoder(n_features=2)
    print(f"Parameters: {sum(p.numel() for p in model.parameters())}\n")

    model, train_losses = train(
        model, train_loader, val_loader, epochs=200, lr=1e-4, device=device, patience=5
    )

    torch.save(model.state_dict(), "autoencoder.pth")
    print("\nModel saved to autoencoder.pth")

    # export to weights.h
    with open("weights.h", "w") as f:
        f.write("#pragma once\n\n")
        for name, param in model.named_parameters():
            c_name = name.replace(".", "_")
            f.write(to_c_array(param, c_name))
            f.write("\n")
