#ifndef BITCOIN_SCRIPT_MULTISIG_H
#define BITCOIN_SCRIPT_MULTISIG_H

#include "script/script.h"
#include "pubkey.h"
#include <vector>

/** Extract public keys and required signers from a multisig redeem script */
bool ExtractPubKeys(const CScript& script, int& required, std::vector<CPubKey>& pubkeys)
{
    pubkeys.clear();
    opcodetype opcode;
    std::vector<unsigned char> data;
    CScript::const_iterator pc = script.begin();

    // Get required signers
    if (!script.GetOp(pc, opcode))
        return false;
    if (opcode < OP_1 || opcode > OP_16)
        return false;
    required = CScript::DecodeOP_N(opcode);

    // Extract public keys
    while (script.GetOp(pc, opcode, data)) {
        if (data.size() == 33) {
            CPubKey pubKey(data.begin(), data.end());
            if (!pubKey.IsValid())
                return false;
            pubkeys.push_back(pubKey);
        }
    }

    // Check final opcode and validate key count
    if (pubkeys.empty() || pubkeys.size() > MAX_PUBKEYS_PER_MULTISIG)
        return false;

    // Get total signers
    if (!script.GetOp(pc, opcode))
        return false;
    if (opcode < OP_1 || opcode > OP_16)
        return false;
    int total = CScript::DecodeOP_N(opcode);

    // Validate script format
    if (total != (int)pubkeys.size() || required > total)
        return false;

    // Check for OP_CHECKMULTISIG
    if (!script.GetOp(pc, opcode) || opcode != OP_CHECKMULTISIG)
        return false;

    // No more opcodes should be present
    if (script.GetOp(pc, opcode))
        return false;

    return true;
}

#endif // BITCOIN_SCRIPT_MULTISIG_H
