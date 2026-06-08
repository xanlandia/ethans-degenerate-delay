# Ethan's Degenerate Delay

A bitcrushing delay VST3/AU by Dirtbag Audio. Each echo tap progressively loses bit depth.

---

## Get the compiled plugin (no coding required)

The plugin builds automatically in the cloud via GitHub Actions every time code is pushed.
Follow these steps **once** to set it up, then downloads are one click forever.

### Step 1 — Create a free GitHub account
Go to https://github.com/signup if you don't have one.

### Step 2 — Create a new repository
1. Go to https://github.com/new
2. Name it `ethans-degenerate-delay`
3. Set it to **Public**
4. Do **not** add a README or .gitignore (the repo should be empty)
5. Click **Create repository**

### Step 3 — Upload the files
On the next page, click **"uploading an existing file"** link.

Drag and drop **everything inside this zip** into the upload area:
```
.github/
.gitmodules
.gitignore
CMakeLists.txt
Source/
```
> Make sure the `.github` folder is included — it contains the build instructions.

Scroll down, click **Commit changes**.

### Step 4 — Wait ~10 minutes for the build
1. Click the **Actions** tab at the top of your repo
2. You'll see a workflow called **"Build VST3"** running (yellow spinner)
3. Wait for it to turn green ✓

### Step 5 — Download your plugin
1. Click the **Releases** link on the right sidebar of your repo
2. You'll see **"Ethan's Degenerate Delay v1.0.x"**
3. Download:
   - `EthansDegenDelay-macOS.zip` → for Mac
   - `EthansDegenDelay-Windows.zip` → for Windows

### Step 6 — Install

**macOS:**
1. Unzip the file
2. Copy `Ethan's Degenerate Delay.vst3` → `~/Library/Audio/Plug-Ins/VST3/`
3. Copy `Ethan's Degenerate Delay.component` → `~/Library/Audio/Plug-Ins/Components/`
4. Restart your DAW and scan for plugins

**Windows:**
1. Unzip the file
2. Copy the `Ethan's Degenerate Delay.vst3` folder → `C:\Program Files\Common Files\VST3\`
3. Restart your DAW and scan for plugins

---

## Parameters

| Knob | Range | What it does |
|------|-------|-------------|
| Time ms | 50–900ms | Delay repeat time |
| Feedback | 0–90% | Echo volume decay |
| Mix | 0–100% | Wet/dry blend |
| Start Bits | 4–16 | Bit depth of first echo |
| Bit Decay | 1–6 | Bits lost per echo tap |
| Srate ÷ | ÷1–÷8 | Sample rate crush per tap |
