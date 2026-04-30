# 2026-04-22T11:07:46.924530200
import vitis

client = vitis.create_client()
client.set_workspace(path="zenith_fft_1d")

vitis.dispose()

