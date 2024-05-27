import ecdsa
from ecdsa import BadSignatureError
from hashlib import sha256


def verify_signature(pub_key_x, pub_key_y, sig, message):
    # Combine the x and y components of the public key
    pub_key = bytes.fromhex(pub_key_x + pub_key_y)

    try:
        # Verify the signature
        vk = ecdsa.VerifyingKey.from_string(
            pub_key, curve=ecdsa.NIST256p, hashfunc=sha256
        )  # the default is sha1
        vk.verify(sig, message)  # True
        print("Signature is valid!")
        return True
    except BadSignatureError:
        print("Signature is invalid!")
        return False


if __name__ == "__main__":
    pub_key_x = "79750c3d48b3068b23138eeca3fab6d68f3c93517e342b819b29c6b4593e2559"
    pub_key_y = "102ff3736c616fcfa457feed056b906d682d40d90fc349e09620fc7b1091af0e"
    signature = bytes.fromhex(
        "27cace14fd5b64b70ab4cfba6cf3dfe2997189b91c6721775c9d134c11ee33445bd9f51b7b5942ac8b04fe2b057298061b4d0f1189d0742dc414f873eb27e283"
    )
    message = b"ba8c9eac092a503e4fb70771c34a00c5bb651043de24db4d3525ebbb3ee7ff08"

    verify_signature(pub_key_x, pub_key_y, signature, message)
