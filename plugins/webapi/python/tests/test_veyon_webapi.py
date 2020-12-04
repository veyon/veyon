#!/usr/bin/python3

import io
import json
import unittest
import time

from PIL import Image
from veyon import WebAPIClient

class Test_VeyonWebAPI(unittest.TestCase):
	def setUp(self):
		self.client = WebAPIClient('http://localhost:11080/api/v1/', 'localhost')
		assert self.client.authenticate_with_key_file('teacher'), "Authentication failed!"

	def tearDown(self):
		self.client.close_connection()

	def test_supported_auth_methods(self):
		methods = self.client.supported_authentication_methods()
		assert (len(methods) > 0), "Server did not report any supported authentication method"

	def test_framebuffer(self):
		c = self.client
		assert c.wait_for_framebuffer(), "Framebuffer not available!"

		img = Image.open(io.BytesIO(c.get_image(format='png', compression='3')))
		assert img.size == (640, 480), "Framebuffer image size mismatch"
		assert img.mode == "RGB", "Framebuffer image format mismatch"
		assert img.getpixel((0,0)) == (0x19, 0x8c, 0xb3), "Framebuffer image pixel mismatch"

		img = Image.open(io.BytesIO(c.get_image(format='jpg', quality=100)))
		assert img.size == (640, 480), "Framebuffer image size mismatch"
		assert img.mode == "RGB", "Framebuffer image format mismatch"
		# exact matching not possible due to lossy JPEG encoding so just check color components of pixel for plausibility
		p = img.getpixel((0,0))
		assert p[0] > 0x10 and p[0] < 0x29 and p[1] > 0x80 and p[1] < 0x99 and p[2] > 0xb0 and p[2] <= 0xb9, "Framebuffer image pixel mismatch"

		assert Image.open(io.BytesIO(c.get_image(format='png', width=160))).size == (160, 120)
		assert Image.open(io.BytesIO(c.get_image(format='png', height=120))).size == (160, 120)
		assert Image.open(io.BytesIO(c.get_image(format='png', width=100, height=100))).size == (100, 100)

	def test_user_information(self):
		assert self.client.wait_for_framebuffer(), "Framebuffer not available!"

		user_information = self.client.user_information()
		assert len(user_information['login']) > 0
		assert 'fullName' in user_information
		assert 'session' in user_information

	def test_features(self):
		available_features = self.client.available_features()
		uids = [value['uid'] for value in available_features]
		assert WebAPIClient.FEATURE_SCREEN_LOCK in uids
		assert WebAPIClient.FEATURE_LOCK_INPUT_DEVICES in uids
		assert WebAPIClient.FEATURE_LOGOFF_USER in uids
		assert WebAPIClient.FEATURE_REBOOT in uids
		assert WebAPIClient.FEATURE_POWER_DOWN in uids
		assert WebAPIClient.FEATURE_DEMO_SERVER in uids
		assert WebAPIClient.FEATURE_WINDOW_DEMO_CLIENT in uids
		assert WebAPIClient.FEATURE_FULLSCREEN_DEMO_CLIENT in uids

		token = self.client.start_demo_server()
		assert token, "Failed to start demo server"

		i = 0
		while i < 30 and not self.client.is_feature_active(WebAPIClient.FEATURE_DEMO_SERVER):
			i += 1
			time.sleep(1)
		assert self.client.is_feature_active(WebAPIClient.FEATURE_DEMO_SERVER), "Demo server feature not active"

		self.client.stop_demo_server()

if "__main__" == __name__:
	unittest.main()
