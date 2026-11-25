# Public-repository Windows build 0.3

Public GitHub Actions is allowed. Do not attach private native source as a public release or commit. The workflow reconstructs the exact v0.3 source ZIP from three owner-provisioned Actions secrets (22,000 / 22,000 / 21,056 base64 characters), checks SHA256, and builds pinned JUCE 8.0.12 on Windows x64. It runs all native tests before packaging. Same-repository feature branch only; fork PRs excluded. Public logs contain status only; private compiler/test diagnostics and VST3 artifacts are 7-Zip AES-encrypted with encrypted headers. No signing, pluginval or FL Studio validation yet.

One-time owner setup: download the private source ZIP (not a public asset), save tools/setup-public-build.ps1 beside it, install GitHub CLI and authenticate with gh auth login. Run the script in PowerShell. It checks the package hash, stores three chunks and an archive password as Actions secrets, and saves the password only in LOCALAPPDATA/Skellay/build-password.txt. Keep that file safe; use it to open the downloaded .7z in 7-Zip. Never post the password or source secrets publicly. Rerun the latest PR workflow after setup. Optionally pass -RunId with the failed run ID to the setup script.

Expected SHA256: a371652f647a57eac6387e96fa263eb92d46dbc4f8792b3524dbe47169cea873.

Local native release and sanitizer suites previously passed 4/4; JUCE wrapper remains uncompiled pending source provisioning and CI. Engine exchange crossfades for 20 ms and resets old tails, not full tail preservation. Screenshots deferred at owner request. Skell Delay untouched.
