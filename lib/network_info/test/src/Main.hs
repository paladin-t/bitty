module Main (main) where

import Network.Info

main = do
    ns <- getNetworkInterfaces
    mapM (putStrLn . showInterface) ns

showInterface :: NetworkInterface -> String
showInterface n = name n ++ "\n"
               ++ "IPv4 Address: " ++ show (ipv4 n) ++ "\n"
               ++ "IPv6 Address: " ++ show (ipv6 n) ++ "\n"
               ++ "MAC Address:  " ++ show (mac n) ++ "\n"
