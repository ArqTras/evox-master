// Copyright (c) 2021-2021, The EVOLUTION Project
// Copyright (c) 2014-2020, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "string_tools.h"
#include "storages/portable_storage_template_helper.h" // epee json include
#include "serialization/keyvalue_serialization.h"

using namespace epee;

#undef EVOLUTION_DEFAULT_LOG_CATEGORY
#define EVOLUTION_DEFAULT_LOG_CATEGORY "checkpoints"

namespace cryptonote
{
  /**
   * @brief struct for loading a checkpoint from json
   */
  struct t_hashline
  {
    uint64_t height; //!< the height of the checkpoint
    std::string hash; //!< the hash for the checkpoint
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(height)
          KV_SERIALIZE(hash)
        END_KV_SERIALIZE_MAP()
  };

  /**
   * @brief struct for loading many checkpoints from json
   */
  struct t_hash_json {
    std::vector<t_hashline> hashlines; //!< the checkpoint lines from the file
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(hashlines)
        END_KV_SERIALIZE_MAP()
  };

  //---------------------------------------------------------------------------
  checkpoints::checkpoints()
  {
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str)
  {
    crypto::hash h = crypto::null_hash;
    bool r = epee::string_tools::hex_to_pod(hash_str, h);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse checkpoint hash string into binary representation!");

    // return false if adding at a height we already have AND the hash is different
    if (m_points.count(height))
    {
      CHECK_AND_ASSERT_MES(h == m_points[height], false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
    }
    m_points[height] = h;
    return true;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_in_checkpoint_zone(uint64_t height) const
  {
    return !m_points.empty() && (height <= (--m_points.end())->first);
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h, bool& is_a_checkpoint) const
  {
    auto it = m_points.find(height);
    is_a_checkpoint = it != m_points.end();
    if(!is_a_checkpoint)
      return true;

    if(it->second == h)
    {
      MINFO("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h);
      return true;
    }
    else
    {
      MWARNING("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
      return false;
    }
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h) const
  {
    bool ignored;
    return check_block(height, h, ignored);
  }
  //---------------------------------------------------------------------------
  //FIXME: is this the desired behavior?
  bool checkpoints::is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const
  {
    if (0 == block_height)
      return false;

    auto it = m_points.upper_bound(blockchain_height);
    // Is blockchain_height before the first checkpoint?
    if (it == m_points.begin())
      return true;

    --it;
    uint64_t checkpoint_height = it->first;
    return checkpoint_height < block_height;
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_max_height() const
  {
    std::map<uint64_t, crypto::hash>::const_iterator highest = std::max_element(m_points.begin(), m_points.end(),
             (boost::bind(&std::map<uint64_t, crypto::hash>::value_type::first, boost::placeholders::_1) <
              boost::bind(&std::map<uint64_t, crypto::hash>::value_type::first, boost::placeholders::_2)));

    return highest->first;
  }
  //---------------------------------------------------------------------------
  const std::map<uint64_t, crypto::hash>& checkpoints::get_points() const
  {
    return m_points;
  }

  bool checkpoints::check_for_conflicts(const checkpoints& other) const
  {
    for (auto& pt : other.get_points())
    {
      if (m_points.count(pt.first))
      {
        CHECK_AND_ASSERT_MES(pt.second == m_points.at(pt.first), false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
      }
    }
    return true;
  }

  bool checkpoints::init_default_checkpoints(network_type nettype)
  {
    if (nettype == TESTNET)
    {
//      ADD_CHECKPOINT(0, "60077b4d5cd49a1278d448c58b6854993d127fcaedbdeab82acff7f7fd86e328");
      return true;
    }
    if (nettype == STAGENET)
    {
//      ADD_CHECKPOINT(0, "60077b4d5cd49a1278d448c58b6854993d127fcaedbdeab82acff7f7fd86e328");
      return true;
    }
    ADD_CHECKPOINT(0, "20c1047c2411b076855977031bf8ccaed4bf544cd03cbc7dbebfef95891248a5");
    ADD_CHECKPOINT(100, "5cce2e92c09a7c8a4a2a4100b94259046fce320115b20de5bb160697885b8c64");
    ADD_CHECKPOINT(200, "17287e3ad525a10a26d2e578be1083e5f9e99395d6fc2b2d9f5612413c46b8a1");
    ADD_CHECKPOINT(300, "242fba325963ffd574ade9021a32884b88c7096ec34a405f74bcd229eed463cf");
    ADD_CHECKPOINT(400, "9e0d9c8beb40720be43c5e51d357b70b9b03d9f4883fc983000b6c06480fa89a");
    ADD_CHECKPOINT(500, "62534533a6a66ba2357f327b2fdb584a8d9d860fb136c1a6303cfe9ff89c2d37");
    ADD_CHECKPOINT(600, "333feca60e428be7f0b693f7037e0c49f82dab3d6cd4f3f9f188a089852cb215");
    ADD_CHECKPOINT(700, "5f6f97b29e9a76e3b5b3c6d554fb0130ee30d238db4f9673017e5735f8f6b906");
    ADD_CHECKPOINT(800, "78803f7183211fc863cf80ecc28f233f46a4276082ce1ada7a87c55d8e6830a6");
    ADD_CHECKPOINT(900, "21d8e9b81188861512ddb81a726f2e5e187e2e2056b62060c320ac8dffb65cee");
    ADD_CHECKPOINT(1000, "09ed61ccdfdd16fdca5f9135a713992b54f8b75b67bb18464168129f3017b9df");
    ADD_CHECKPOINT(1100, "e46cdc377f9178b26a20b7b34c70eb14d403a495e94538a3fd2c8fc913a23fd8");
    ADD_CHECKPOINT(1200, "ff3c506d482f6b44077255c8e9c680e5a10fb6689e6ec2933e580804e1f306c9");
    ADD_CHECKPOINT(1300, "61c0c8a792b5d10b913f956235fcbb80dced65bc7ab0d426e2d61ff419fa4a02");
    ADD_CHECKPOINT(1400, "15b8f551f5ffcf7191bd4821e164976a6bde0a655f8b099a2bed965a41eac441");
    ADD_CHECKPOINT(1500, "68bd182d04dbd1764faae7255a6ac9b240bb526c361fb63c178ee02fa0ed1fd9");
    ADD_CHECKPOINT(1600, "6383d0ac0d1b1c78e1bd02c55d75a6d3eac27e28ac6b047f626dcb17fb80cb29");
    ADD_CHECKPOINT(1700, "8bfebd680da7675218173ff58ab1fb12355ea7a7f10e36be5f2ebb4dd5dd73f2");
    ADD_CHECKPOINT(1800, "2fa3aa2cbae9a354cf8accc39ba2aaae0e605ccf938197b13b98487affe98482");
    ADD_CHECKPOINT(1900, "d1e258005c45e103da748ed8c06f8690d0bccdcaccb3ecd457f6f420c1af130f");
    ADD_CHECKPOINT(2000, "bbdb1da79b03a0b87115152c332f0160c712bdf1c36c4af725afb5efce6baf2e");
    ADD_CHECKPOINT(2100, "9a478edf1c8f90dc869773bdfdd6d7d3ca671d8dd86254ef1b728d027e7e197f");
    ADD_CHECKPOINT(2200, "8e51174bca1180da4aa1df8a28dd72dce60cb89cb580e92c223c91ad3cd85a6a");
    ADD_CHECKPOINT(2300, "cb3ad658ce51c9917b21c4bf28408cef9d965f8fa0f08d4fa761d3d709d55a09");
    ADD_CHECKPOINT(2400, "39abe9246f343930f4f5a7d8abc6ecd588d8ad9fc3a892747e6a068119a5919b");
    ADD_CHECKPOINT(2500, "b8d436665e3605bcecdbaa1259c1994e723b0408592a552cbc8e9dcf765cc752");
    ADD_CHECKPOINT(5000, "1892347da859213e4c09c2d82888e86679d6ba39a5f0afbbd08516c8987947f2");
    ADD_CHECKPOINT(10000, "9eada6de6ca163f41ad77c9364913f6788403bf38f183eaff27dec53768a0351");
    ADD_CHECKPOINT(10500, "7bd671c6d1be3e793afa4f626382aef26d3c645e27f8c038a85372dd5976bad9");
    ADD_CHECKPOINT(11500, "07db8815f9254f2b6eab1c9845a30efeafdf93d451573e36b385af11b2f55a0d");
    ADD_CHECKPOINT(12500, "8e775f7419af889476d4d26bf7cfebd477140c030ade0707c35cc9798b150ff9");
    ADD_CHECKPOINT(15000, "706d1eaaa2d95fb82ecee3bdeda61e48ac9182d7a8262df4aef69a7d3ceda74d");
    ADD_CHECKPOINT(30000, "adaf206bda8190d7805691ab278b631e1580c0e8eb43efc5929efa72ef7aad8b");
    ADD_CHECKPOINT(40000, "12deea9f814ea1853aee5247a79a3563fcff861bd9188be2e4d63609d8d4ef6a");
    ADD_CHECKPOINT(50000, "84d3703de9ab502f5af86cb14c2578ed9c78369ac359a7e13897448e2110595f");
    ADD_CHECKPOINT(70000, "83c7bd1af221e75c459716f2944b72348fb63d2b97c3ff82bc20d0e5b8b35591");
    ADD_CHECKPOINT(90000, "e6d35d9b788e2e6acbf1e26cf36e58f5372b3a4787822d1e2c5ba2fcd6993e5a");
    ADD_CHECKPOINT(100000, "da884b94a77b678b4e054d04a417e24fc587be66bf6c06e5752fbaca5b4df80a");
    ADD_CHECKPOINT(120000, "f02afadd100c3b2d7d016ae59f72d1c167b044a48218f37799dedb6bc4f24f0b");
    ADD_CHECKPOINT(150000, "8c2bb1970d1ee8c953db03d63d8b3615550890b13feb7a7971bf3cf3c41684fe");
    ADD_CHECKPOINT(170000, "f46318407d557ba8c15d1e2928bee87f2ea1d59ef8f946c3af4621fa0b760126");
    ADD_CHECKPOINT(180000, "0fbcd5bd616fe0f5803332c9169782ae5e3d7f8747641d94ec0272f98eaa6f12");
    ADD_CHECKPOINT(190000, "5be3d683a1ff212d03fa45851fea36ef5026c1d1f1034aa76599d1d94f54ea10");
    ADD_CHECKPOINT(200000, "7afd6552d07c7b118aff99cae81eb472ec23daca7e91234a56aa46a98cdb059f");
    ADD_CHECKPOINT(210000, "77f1cfb434d4bc2163ce76e6c4125532c2ecf4a28049c56b109e7e341c2aabb1");
    ADD_CHECKPOINT(215000, "1a1953dcf4e071be6582ff8b8bb1007516550902de0aa5c0f20b2985f9dfd247");
    ADD_CHECKPOINT(220000, "2663809960913363373c9230db6a779f956cc95c86d90b08c74d1f8d23fb9e19");
    ADD_CHECKPOINT(225000, "baad9c11991fe9a81abcc820cc04cad2b87cedcfd9ae07ec9c237b969bcd4d15");
    ADD_CHECKPOINT(230000, "87ecf38629ae7d8812e1cd9c362eb0cd4c29297405ff860e390302e350f2746b");
    ADD_CHECKPOINT(235000, "854611792bf93e0dcc2c49e3fd3c5d081e12c776bfd2ee6f39327fb4af12db40");
    ADD_CHECKPOINT(240000, "ee9ed37e1b73bea29a6aeccd6fa2ee09be40109c9d7205365f15e3db3610e38b");
    ADD_CHECKPOINT(240500, "e3e6b8499422ef26fdb0e35ace6e758cd90f8f41c73307569e3329ca1011d5a8");
    ADD_CHECKPOINT(241000, "4067b2cac60e26c2c12f98a0c06dc02f2780ea1656f79417882241eb4ccd9632");
    ADD_CHECKPOINT(241500, "346734dbb514ea6555b892732e247f5e61c1b072c59dd54184f03b4fda7bbae2");
    ADD_CHECKPOINT(242000, "1842bd62b46ff98c531d9a455308045e1cde2eec15720f594e8a4021d904304e");
    ADD_CHECKPOINT(242500, "b466d103adc217fa82197724ac9af1d50fc853a7b58165acfa8f0c38646014de");
    ADD_CHECKPOINT(243000, "51a22e6326aadb5b727994c353f336e3b29edb1b2d487a5b3aacad23ca6b9d8c");
    ADD_CHECKPOINT(243500, "06a44da7ffabebae3d995c7d6c6a70039b88e5f653a10d0cdeb7f240e85bf756");
    return true;
  }

  bool checkpoints::load_checkpoints_from_json(const std::string &json_hashfile_fullpath)
  {
    boost::system::error_code errcode;
    if(!(boost::filesystem::exists(json_hashfile_fullpath, errcode)))
    {
      LOG_PRINT_L1("Blockchain checkpoints file not found");
      return true;
    }

    LOG_PRINT_L1("Adding checkpoints from blockchain hashfile");

    uint64_t prev_max_height = get_max_height();
    LOG_PRINT_L1("Hard-coded max checkpoint height is " << prev_max_height);
    t_hash_json hashes;
    if(!epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath))
    {
      MERROR("Error loading checkpoints from " << json_hashfile_fullpath);
      return false;
    }
    for(std::vector<t_hashline>::const_iterator it = hashes.hashlines.begin(); it != hashes.hashlines.end(); )
    {
      uint64_t height;
      height = it->height;
      if (height <= prev_max_height) {
	LOG_PRINT_L1("ignoring checkpoint height " << height);
      } else {
	std::string blockhash = it->hash;
	LOG_PRINT_L1("Adding checkpoint height " << height << ", hash=" << blockhash);
	ADD_CHECKPOINT(height, blockhash);
      }
      ++it;
    }

    return true;
  }

  bool checkpoints::load_checkpoints_from_dns(network_type nettype)
  {
    std::vector<std::string> records;

    // All four EVOLUTION Network domains have DNSSEC on and valid
    static const std::vector<std::string> dns_urls = {

	};

    static const std::vector<std::string> testnet_dns_urls = {
    };

    static const std::vector<std::string> stagenet_dns_urls = {
    };

    if (!tools::dns_utils::load_txt_records_from_dns(records, nettype == TESTNET ? testnet_dns_urls : nettype == STAGENET ? stagenet_dns_urls : dns_urls))
      return false; // why true ?

    for (const auto& record : records)
    {
      auto pos = record.find(":");
      if (pos != std::string::npos)
      {
        uint64_t height;
        crypto::hash hash;

        // parse the first part as uint64_t,
        // if this fails move on to the next record
        std::stringstream ss(record.substr(0, pos));
        if (!(ss >> height))
        {
    continue;
        }

        // parse the second part as crypto::hash,
        // if this fails move on to the next record
        std::string hashStr = record.substr(pos + 1);
        if (!epee::string_tools::hex_to_pod(hashStr, hash))
        {
    continue;
        }

        ADD_CHECKPOINT(height, hashStr);
      }
    }
    return true;
  }

  bool checkpoints::load_new_checkpoints(const std::string &json_hashfile_fullpath, network_type nettype, bool dns)
  {
    bool result;

    result = load_checkpoints_from_json(json_hashfile_fullpath);
    if (dns)
    {
      result &= load_checkpoints_from_dns(nettype);
    }

    return result;
  }
}
