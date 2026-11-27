# v0.4 state recall integration
Native state codec extracted from JUCE wrapper and tested. Exact same v2 956-byte wire format. v1 restore changes only its own engine, v2 restore changes all three atomically, invalid inputs rejected. Original v15 Bank bytes retained. Bank editor no longer clamps display of valid imported pattern steps above three; 128 is an experimental UI ceiling, expanded for larger imported values, not a verified FL limit.

Local GCC release 5/5 tests (1.02 s); ASan/UBSan/float-cast-overflow 5/5 (13.56 s). Three engine round trips, 2868 truncated inputs, v1 migration and v15 preservation passed. These are native tests, not DAW recall or JUCE compile.

CI applies public integration overlay to original private v0.3 ZIP using same existing secrets. No source re-provisioning needed if configured. Windows still depends on owner-provisioned Actions secrets; local JUCE fetch blocked by DNS. Screenshots deferred. No merge or release. Skell Delay untouched.
