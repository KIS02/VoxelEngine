Shadow Bias Probe for Phase 11.6 - DIAGNOSTIC ONLY

This patch preserves the prior resolution-only trial (OpenGL 1024, Software 512) and adds a switchable high-bias diagnostic. It is NOT an intended final shadow quality configuration.

Set $env:VOXELENGINE_DEMO_SHADOW='1' to enable scene shadows.
Set $env:VOXELENGINE_SHADOW_BIAS_PROBE='0' to use default bias.
Set $env:VOXELENGINE_SHADOW_BIAS_PROBE='1' to use deliberately exaggerated bias 0.01 + 0.03*slope.

Each backend prints a [ShadowProbe] line once per process, including actual resolution and bias.
If pattern changes or shadows detach under Probe=1, depth comparison/bias is a key issue.
If Probe=1 produces no difference, verify logs and then inspect shadow coordinates and buffer.

MSVC may show C4996 warnings for std::getenv, as already observed in DemoScene.
