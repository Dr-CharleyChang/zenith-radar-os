# 2026-03-11T15:25:25.189422200
import vitis

client = vitis.create_client()
client.set_workspace(path="cfar")

comp = client.create_hls_component(name = "cfar",cfg_file = ["hls_config.cfg"],template = "empty_hls_component")

comp = client.get_component(name="cfar")
comp.run(operation="C_SIMULATION")

comp.run(operation="SYNTHESIS")

vitis.dispose()

