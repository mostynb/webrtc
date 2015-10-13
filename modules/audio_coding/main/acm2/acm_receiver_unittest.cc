/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_coding/main/acm2/acm_receiver.h"

#include <algorithm>  // std::min

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/main/acm2/audio_coding_module_impl.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/neteq/tools/rtp_generator.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/test/test_suite.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace webrtc {

namespace acm2 {
namespace {

bool CodecsEqual(const CodecInst& codec_a, const CodecInst& codec_b) {
    if (strcmp(codec_a.plname, codec_b.plname) != 0 ||
        codec_a.plfreq != codec_b.plfreq ||
        codec_a.pltype != codec_b.pltype ||
        codec_b.channels != codec_a.channels)
      return false;
    return true;
}

}  // namespace

class AcmReceiverTest : public AudioPacketizationCallback,
                        public ::testing::Test {
 protected:
  AcmReceiverTest()
      : timestamp_(0),
        packet_sent_(false),
        last_packet_send_timestamp_(timestamp_),
        last_frame_type_(kFrameEmpty) {
    AudioCoding::Config config;
    config.transport = this;
    acm_.reset(new AudioCodingImpl(config));
    receiver_.reset(new AcmReceiver(config.ToOldConfig()));
  }

  ~AcmReceiverTest() {}

  void SetUp() override {
    ASSERT_TRUE(receiver_.get() != NULL);
    ASSERT_TRUE(acm_.get() != NULL);
    for (int n = 0; n < ACMCodecDB::kNumCodecs; n++) {
      ASSERT_EQ(0, ACMCodecDB::Codec(n, &codecs_[n]));
    }

    rtp_header_.header.sequenceNumber = 0;
    rtp_header_.header.timestamp = 0;
    rtp_header_.header.markerBit = false;
    rtp_header_.header.ssrc = 0x12345678;  // Arbitrary.
    rtp_header_.header.numCSRCs = 0;
    rtp_header_.header.payloadType = 0;
    rtp_header_.frameType = kAudioFrameSpeech;
    rtp_header_.type.Audio.isCNG = false;
  }

  void TearDown() override {}

  void InsertOnePacketOfSilence(int codec_id) {
    CodecInst codec;
    ACMCodecDB::Codec(codec_id, &codec);
    if (timestamp_ == 0) {  // This is the first time inserting audio.
      ASSERT_TRUE(acm_->RegisterSendCodec(codec_id, codec.pltype));
    } else {
      const CodecInst* current_codec = acm_->GetSenderCodecInst();
      ASSERT_TRUE(current_codec);
      if (!CodecsEqual(codec, *current_codec))
        ASSERT_TRUE(acm_->RegisterSendCodec(codec_id, codec.pltype));
    }
    AudioFrame frame;
    // Frame setup according to the codec.
    frame.sample_rate_hz_ = codec.plfreq;
    frame.samples_per_channel_ = codec.plfreq / 100;  // 10 ms.
    frame.num_channels_ = codec.channels;
    memset(frame.data_, 0, frame.samples_per_channel_ * frame.num_channels_ *
           sizeof(int16_t));
    int num_bytes = 0;
    packet_sent_ = false;
    last_packet_send_timestamp_ = timestamp_;
    while (num_bytes == 0) {
      frame.timestamp_ = timestamp_;
      timestamp_ += frame.samples_per_channel_;
      num_bytes = acm_->Add10MsAudio(frame);
      ASSERT_GE(num_bytes, 0);
    }
    ASSERT_TRUE(packet_sent_);  // Sanity check.
  }

  // Last element of id should be negative.
  void AddSetOfCodecs(const int* id) {
    int n = 0;
    while (id[n] >= 0) {
      ASSERT_EQ(0, receiver_->AddCodec(id[n], codecs_[id[n]].pltype,
                                       codecs_[id[n]].channels,
                                       codecs_[id[n]].plfreq, NULL));
      ++n;
    }
  }

  int32_t SendData(FrameType frame_type,
                   uint8_t payload_type,
                   uint32_t timestamp,
                   const uint8_t* payload_data,
                   size_t payload_len_bytes,
                   const RTPFragmentationHeader* fragmentation) override {
    if (frame_type == kFrameEmpty)
      return 0;

    rtp_header_.header.payloadType = payload_type;
    rtp_header_.frameType = frame_type;
    if (frame_type == kAudioFrameSpeech)
      rtp_header_.type.Audio.isCNG = false;
    else
      rtp_header_.type.Audio.isCNG = true;
    rtp_header_.header.timestamp = timestamp;

    int ret_val = receiver_->InsertPacket(rtp_header_, payload_data,
                                          payload_len_bytes);
    if (ret_val < 0) {
      assert(false);
      return -1;
    }
    rtp_header_.header.sequenceNumber++;
    packet_sent_ = true;
    last_frame_type_ = frame_type;
    return 0;
  }

  rtc::scoped_ptr<AcmReceiver> receiver_;
  CodecInst codecs_[ACMCodecDB::kMaxNumCodecs];
  rtc::scoped_ptr<AudioCoding> acm_;
  WebRtcRTPHeader rtp_header_;
  uint32_t timestamp_;
  bool packet_sent_;  // Set when SendData is called reset when inserting audio.
  uint32_t last_packet_send_timestamp_;
  FrameType last_frame_type_;
};

TEST_F(AcmReceiverTest, DISABLED_ON_ANDROID(AddCodecGetCodec)) {
  // Add codec.
  for (int n = 0; n < ACMCodecDB::kNumCodecs; ++n) {
    if (n & 0x1)  // Just add codecs with odd index.
      EXPECT_EQ(0,
                receiver_->AddCodec(n, codecs_[n].pltype, codecs_[n].channels,
                                    codecs_[n].plfreq, NULL));
  }
  // Get codec and compare.
  for (int n = 0; n < ACMCodecDB::kNumCodecs; ++n) {
    CodecInst my_codec;
    if (n & 0x1) {
      // Codecs with odd index should match the reference.
      EXPECT_EQ(0, receiver_->DecoderByPayloadType(codecs_[n].pltype,
                                                   &my_codec));
      EXPECT_TRUE(CodecsEqual(codecs_[n], my_codec));
    } else {
      // Codecs with even index are not registered.
      EXPECT_EQ(-1, receiver_->DecoderByPayloadType(codecs_[n].pltype,
                                                    &my_codec));
    }
  }
}

TEST_F(AcmReceiverTest, DISABLED_ON_ANDROID(AddCodecChangePayloadType)) {
  const int codec_id = ACMCodecDB::kPCMA;
  CodecInst ref_codec1;
  EXPECT_EQ(0, ACMCodecDB::Codec(codec_id, &ref_codec1));
  CodecInst ref_codec2 = ref_codec1;
  ++ref_codec2.pltype;
  CodecInst test_codec;

  // Register the same codec with different payload types.
  EXPECT_EQ(
      0, receiver_->AddCodec(codec_id, ref_codec1.pltype, ref_codec1.channels,
                             ref_codec1.plfreq, NULL));
  EXPECT_EQ(
      0, receiver_->AddCodec(codec_id, ref_codec2.pltype, ref_codec2.channels,
                             ref_codec2.plfreq, NULL));

  // Both payload types should exist.
  EXPECT_EQ(0, receiver_->DecoderByPayloadType(ref_codec1.pltype, &test_codec));
  EXPECT_EQ(true, CodecsEqual(ref_codec1, test_codec));
  EXPECT_EQ(0, receiver_->DecoderByPayloadType(ref_codec2.pltype, &test_codec));
  EXPECT_EQ(true, CodecsEqual(ref_codec2, test_codec));
}

TEST_F(AcmReceiverTest, DISABLED_ON_ANDROID(AddCodecChangeCodecId)) {
  const int codec_id1 = ACMCodecDB::kPCMU;
  CodecInst ref_codec1;
  EXPECT_EQ(0, ACMCodecDB::Codec(codec_id1, &ref_codec1));
  const int codec_id2 = ACMCodecDB::kPCMA;
  CodecInst ref_codec2;
  EXPECT_EQ(0, ACMCodecDB::Codec(codec_id2, &ref_codec2));
  ref_codec2.pltype = ref_codec1.pltype;
  CodecInst test_codec;

  // Register the same payload type with different codec ID.
  EXPECT_EQ(
      0, receiver_->AddCodec(codec_id1, ref_codec1.pltype, ref_codec1.channels,
                             ref_codec1.plfreq, NULL));
  EXPECT_EQ(
      0, receiver_->AddCodec(codec_id2, ref_codec2.pltype, ref_codec2.channels,
                             ref_codec2.plfreq, NULL));

  // Make sure that the last codec is used.
  EXPECT_EQ(0, receiver_->DecoderByPayloadType(ref_codec2.pltype, &test_codec));
  EXPECT_EQ(true, CodecsEqual(ref_codec2, test_codec));
}

TEST_F(AcmReceiverTest, DISABLED_ON_ANDROID(AddCodecRemoveCodec)) {
  CodecInst codec;
  const int codec_id = ACMCodecDB::kPCMA;
  EXPECT_EQ(0, ACMCodecDB::Codec(codec_id, &codec));
  const int payload_type = codec.pltype;
  EXPECT_EQ(0, receiver_->AddCodec(codec_id, codec.pltype, codec.channels,
                                   codec.plfreq, NULL));

  // Remove non-existing codec should not fail. ACM1 legacy.
  EXPECT_EQ(0, receiver_->RemoveCodec(payload_type + 1));

  // Remove an existing codec.
  EXPECT_EQ(0, receiver_->RemoveCodec(payload_type));

  // Ask for the removed codec, must fail.
  EXPECT_EQ(-1, receiver_->DecoderByPayloadType(payload_type, &codec));
}

TEST_F(AcmReceiverTest, DISABLED_ON_ANDROID(SampleRate)) {
  const int kCodecId[] = {
      ACMCodecDB::kISAC, ACMCodecDB::kISACSWB,
      -1  // Terminator.
  };
  AddSetOfCodecs(kCodecId);

  AudioFrame frame;
  const int kOutSampleRateHz = 8000;  // Different than codec sample rate.
  int n = 0;
  while (kCodecId[n] >= 0) {
    const int num_10ms_frames = codecs_[kCodecId[n]].pacsize /
        (codecs_[kCodecId[n]].plfreq / 100);
    InsertOnePacketOfSilence(kCodecId[n]);
    for (int k = 0; k < num_10ms_frames; ++k) {
      EXPECT_EQ(0, receiver_->GetAudio(kOutSampleRateHz, &frame));
    }
    EXPECT_EQ(std::min(32000, codecs_[kCodecId[n]].plfreq),
              receiver_->current_sample_rate_hz());
    ++n;
  }
}

TEST_F(AcmReceiverTest, DISABLED_ON_ANDROID(PostdecodingVad)) {
  receiver_->EnableVad();
  EXPECT_TRUE(receiver_->vad_enabled());

  const int id = ACMCodecDB::kPCM16Bwb;
  ASSERT_EQ(0, receiver_->AddCodec(id, codecs_[id].pltype, codecs_[id].channels,
                                   codecs_[id].plfreq, NULL));
  const int kNumPackets = 5;
  const int num_10ms_frames = codecs_[id].pacsize / (codecs_[id].plfreq / 100);
  AudioFrame frame;
  for (int n = 0; n < kNumPackets; ++n) {
    InsertOnePacketOfSilence(id);
    for (int k = 0; k < num_10ms_frames; ++k)
      ASSERT_EQ(0, receiver_->GetAudio(codecs_[id].plfreq, &frame));
  }
  EXPECT_EQ(AudioFrame::kVadPassive, frame.vad_activity_);

  receiver_->DisableVad();
  EXPECT_FALSE(receiver_->vad_enabled());

  for (int n = 0; n < kNumPackets; ++n) {
    InsertOnePacketOfSilence(id);
    for (int k = 0; k < num_10ms_frames; ++k)
      ASSERT_EQ(0, receiver_->GetAudio(codecs_[id].plfreq, &frame));
  }
  EXPECT_EQ(AudioFrame::kVadUnknown, frame.vad_activity_);
}

#ifdef WEBRTC_CODEC_ISAC
#define IF_ISAC_FLOAT(x) x
#else
#define IF_ISAC_FLOAT(x) DISABLED_##x
#endif

TEST_F(AcmReceiverTest, DISABLED_ON_ANDROID(IF_ISAC_FLOAT(LastAudioCodec))) {
  const int kCodecId[] = {
      ACMCodecDB::kISAC, ACMCodecDB::kPCMA, ACMCodecDB::kISACSWB,
      ACMCodecDB::kPCM16Bswb32kHz,
      -1  // Terminator.
  };
  AddSetOfCodecs(kCodecId);

  const int kCngId[] = {  // Not including full-band.
      ACMCodecDB::kCNNB, ACMCodecDB::kCNWB, ACMCodecDB::kCNSWB,
      -1  // Terminator.
  };
  AddSetOfCodecs(kCngId);

  // Register CNG at sender side.
  int n = 0;
  while (kCngId[n] > 0) {
    ASSERT_TRUE(acm_->RegisterSendCodec(kCngId[n], codecs_[kCngId[n]].pltype));
    ++n;
  }

  CodecInst codec;
  // No audio payload is received.
  EXPECT_EQ(-1, receiver_->LastAudioCodec(&codec));

  // Start with sending DTX.
  ASSERT_TRUE(acm_->SetVad(true, true, VADVeryAggr));
  packet_sent_ = false;
  InsertOnePacketOfSilence(kCodecId[0]);  // Enough to test with one codec.
  ASSERT_TRUE(packet_sent_);
  EXPECT_EQ(kAudioFrameCN, last_frame_type_);

  // Has received, only, DTX. Last Audio codec is undefined.
  EXPECT_EQ(-1, receiver_->LastAudioCodec(&codec));
  EXPECT_EQ(-1, receiver_->last_audio_codec_id());

  n = 0;
  while (kCodecId[n] >= 0) {  // Loop over codecs.
    // Set DTX off to send audio payload.
    acm_->SetVad(false, false, VADAggr);
    packet_sent_ = false;
    InsertOnePacketOfSilence(kCodecId[n]);

    // Sanity check if Actually an audio payload received, and it should be
    // of type "speech."
    ASSERT_TRUE(packet_sent_);
    ASSERT_EQ(kAudioFrameSpeech, last_frame_type_);
    EXPECT_EQ(kCodecId[n], receiver_->last_audio_codec_id());

    // Set VAD on to send DTX. Then check if the "Last Audio codec" returns
    // the expected codec.
    acm_->SetVad(true, true, VADAggr);

    // Do as many encoding until a DTX is sent.
    while (last_frame_type_ != kAudioFrameCN) {
      packet_sent_ = false;
      InsertOnePacketOfSilence(kCodecId[n]);
      ASSERT_TRUE(packet_sent_);
    }
    EXPECT_EQ(kCodecId[n], receiver_->last_audio_codec_id());
    EXPECT_EQ(0, receiver_->LastAudioCodec(&codec));
    EXPECT_TRUE(CodecsEqual(codecs_[kCodecId[n]], codec));
    ++n;
  }
}

}  // namespace acm2

}  // namespace webrtc
