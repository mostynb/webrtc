# Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.
{
  'variables': {
    'webrtc_video_dependencies': [
      '<(webrtc_root)/base/base.gyp:rtc_base_approved',
      '<(webrtc_root)/common.gyp:webrtc_common',
      '<(webrtc_root)/common_video/common_video.gyp:common_video',
      '<(webrtc_root)/modules/modules.gyp:bitrate_controller',
      '<(webrtc_root)/modules/modules.gyp:paced_sender',
      '<(webrtc_root)/modules/modules.gyp:rtp_rtcp',
      '<(webrtc_root)/modules/modules.gyp:video_capture_module',
      '<(webrtc_root)/modules/modules.gyp:video_processing',
      '<(webrtc_root)/modules/modules.gyp:video_render_module',
      '<(webrtc_root)/modules/modules.gyp:webrtc_utility',
      '<(webrtc_root)/modules/modules.gyp:webrtc_video_coding',
      '<(webrtc_root)/system_wrappers/system_wrappers.gyp:system_wrappers',
      '<(webrtc_root)/voice_engine/voice_engine.gyp:voice_engine',
      '<(webrtc_root)/webrtc.gyp:rtc_event_log',
    ],
    'webrtc_video_sources': [
      'video/encoded_frame_callback_adapter.cc',
      'video/encoded_frame_callback_adapter.h',
      'video/receive_statistics_proxy.cc',
      'video/receive_statistics_proxy.h',
      'video/send_statistics_proxy.cc',
      'video/send_statistics_proxy.h',
      'video/video_capture_input.cc',
      'video/video_capture_input.h',
      'video/video_decoder.cc',
      'video/video_encoder.cc',
      'video/video_receive_stream.cc',
      'video/video_receive_stream.h',
      'video/video_send_stream.cc',
      'video/video_send_stream.h',
      'video_engine/call_stats.cc',
      'video_engine/call_stats.h',
      'video_engine/encoder_state_feedback.cc',
      'video_engine/encoder_state_feedback.h',
      'video_engine/overuse_frame_detector.cc',
      'video_engine/overuse_frame_detector.h',
      'video_engine/payload_router.cc',
      'video_engine/payload_router.h',
      'video_engine/report_block_stats.cc',
      'video_engine/report_block_stats.h',
      'video_engine/stream_synchronization.cc',
      'video_engine/stream_synchronization.h',
      'video_engine/vie_channel.cc',
      'video_engine/vie_channel.h',
      'video_engine/vie_defines.h',
      'video_engine/vie_encoder.cc',
      'video_engine/vie_encoder.h',
      'video_engine/vie_receiver.cc',
      'video_engine/vie_receiver.h',
      'video_engine/vie_remb.cc',
      'video_engine/vie_remb.h',
      'video_engine/vie_sync_module.cc',
      'video_engine/vie_sync_module.h',
    ],
  },
}
