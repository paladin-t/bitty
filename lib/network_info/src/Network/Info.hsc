{-# LANGUAGE ForeignFunctionInterface #-}

module Network.Info (
    getNetworkInterfaces,
    NetworkInterface (..),
    IPv4 (..),
    IPv6 (..),
    MAC (..),
) where

import Data.Bits ((.&.), shiftR, shiftL)
import Data.List (intersperse)
import Data.Word
import Foreign.C.String
import Foreign.C.Types
import Foreign.Marshal.Array
import Foreign.Ptr
import Foreign.Storable
import Numeric (showHex)
import Text.Printf


----------------------------------------------------------------------
-- FFI
----------------------------------------------------------------------

#include "network.h"

foreign import ccall unsafe "c_get_network_interfaces"
        c_get_network_interfaces :: Ptr NetworkInterface -> CInt -> IO CInt


----------------------------------------------------------------------
-- Network interfaces
----------------------------------------------------------------------

-- | Describes the basic configuration of a network interface. /This/
--   /definition is currently limited to just one address per family./
data NetworkInterface = NetworkInterface
    { name :: String -- ^ Interface name (e.g. \"eth0\", \"lo\", \"Local Area Connection\")
    , ipv4 :: IPv4   -- ^ IPv4 address
    , ipv6 :: IPv6   -- ^ IPv6 address
    , mac  :: MAC    -- ^ MAC address
    } deriving (Show)

instance Storable NetworkInterface where
    alignment _ = #const offsetof(struct {char x__; struct network_interface (y__); }, y__)
    sizeOf _    = #size struct network_interface
    peek ptr    = do
        name <- peekCWString $ (#ptr struct network_interface, name) ptr
        ipv4 <- (#peek struct network_interface, ip_address) ptr
        ipv6 <- (#peek struct network_interface, ip6_address) ptr
        mac  <- (#peek struct network_interface, mac_address) ptr
        return $ NetworkInterface name ipv4 ipv6 mac


-- | Gets the address information for each of the network interfaces on
--   the local computer.
getNetworkInterfaces :: IO [NetworkInterface]
getNetworkInterfaces =
    allocaArray 64 $ \ptr -> do
    count <- c_get_network_interfaces ptr 64
    peekArray (fromIntegral count) ptr


----------------------------------------------------------------------
-- IPv4 addresses
----------------------------------------------------------------------

-- | Represents an IPv4 address (e.g. @172.23.21.1@, @127.0.0.1@)
data IPv4 = IPv4
    {-# UNPACK #-} !Word32
    deriving (Eq, Ord, Bounded)

instance Show IPv4 where
    show = showIPv4

instance Storable IPv4 where
    alignment _ = 1
    sizeOf _    = 4
    peek p      = do
        ip <- peek (castPtr p)
        return (IPv4 ip)
    poke p (IPv4 ip) =
        poke (castPtr p) ip


----------------------------------------------------------------------
-- IPv6 addresses
----------------------------------------------------------------------

-- | Represents an IPv6 address (e.g. @2001:db8:85a3::8a2e:370:7334@, @::1@)
data IPv6 = IPv6
    {-# UNPACK #-} !Word32
    {-# UNPACK #-} !Word32
    {-# UNPACK #-} !Word32
    {-# UNPACK #-} !Word32
    deriving (Eq, Ord, Bounded)

-- | Not yet capable of collapsing groups of zeros, will still
--   generate valid addresses however.
instance Show IPv6 where
    show = showIPv6

instance Storable IPv6 where
    alignment _ = 1
    sizeOf _    = 16
    peek p      = do
        let ptr = castPtr p
        a <- peekElemOff ptr 0
        b <- peekElemOff ptr 1
        c <- peekElemOff ptr 2
        d <- peekElemOff ptr 3
        return $ IPv6 a b c d
    poke p (IPv6 a b c d) = do
        let ptr = castPtr p
        pokeElemOff ptr 0 a
        pokeElemOff ptr 1 b
        pokeElemOff ptr 2 c
        pokeElemOff ptr 3 d


----------------------------------------------------------------------
-- MAC addresses
----------------------------------------------------------------------

-- | Represents a MAC address (e.g. @01:23:45:67:89:ab@)
data MAC = MAC
    {-# UNPACK #-} !Word8
    {-# UNPACK #-} !Word8
    {-# UNPACK #-} !Word8
    {-# UNPACK #-} !Word8
    {-# UNPACK #-} !Word8
    {-# UNPACK #-} !Word8
    deriving (Eq, Ord, Bounded)

instance Show MAC where
    show (MAC a b c d e f) = printf "%02x:%02x:%02x:%02x:%02x:%02x" a b c d e f

instance Storable MAC where
    alignment _ = 1
    sizeOf _    = 6
    peek p      = do
        a <- peek $ castPtr p
        b <- peekByteOff p 1
        c <- peekByteOff p 2
        d <- peekByteOff p 3
        e <- peekByteOff p 4
        f <- peekByteOff p 5
        return $ MAC a b c d e f
    poke p (MAC a b c d e f) = do
        poke (castPtr p) a
        pokeByteOff p 1 b
        pokeByteOff p 2 c
        pokeByteOff p 3 d
        pokeByteOff p 4 e
        pokeByteOff p 5 f


----------------------------------------------------------------------
-- Helper functions
----------------------------------------------------------------------

showIPv4 :: IPv4 -> String
showIPv4 (IPv4 ip) = concat . intersperse "." $ showOctets
  where
    showOctets = map show $ word8s ip

-- TODO: drop out consecutive zeros
showIPv6 :: IPv6 -> String
showIPv6 (IPv6 a b c d) = (concat . intersperse ":") groups
  where
    groups = map (flip showHex "")  $ concatMap (group . word8s) [a,b,c,d]

word8s :: Word32 -> [Word8]
word8s x = [ fromIntegral $ x
           , fromIntegral $ x `shiftR` 8
           , fromIntegral $ x `shiftR` 16
           , fromIntegral $ x `shiftR` 24 ]

group :: [Word8] -> [Word16]
group = map2 $ \x y -> (fromIntegral x) `shiftL` 8 + (fromIntegral y)

map2 :: (a -> a -> b) -> [a] -> [b]
map2 _ [] = []
map2 f (x:y:zs) = f x y : map2 f zs
