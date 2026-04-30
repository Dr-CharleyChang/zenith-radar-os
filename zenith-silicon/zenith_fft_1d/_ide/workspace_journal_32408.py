# 2026-04-20T17:02:29.397378
import vitis

client = vitis.create_client()
client.set_workspace(path="zenith_fft_1d")

comp = client.create_hls_component(name = "zenith_fft_1d",cfg_file = ["hls_config.cfg"],template = "empty_hls_component")

comp = client.get_component(name="zenith_fft_1d")
comp.run(operation="C_SIMULATION")

vitis.dispose()

