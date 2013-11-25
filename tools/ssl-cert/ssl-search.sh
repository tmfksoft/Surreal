#!/bin/sh

#search for an openssl installation . . . 

    for dir in /usr /usr/local/ssl /usr/local/openssl /usr/local /opt/ssl /opt/openssl; do
        test -x "$dir/bin/openssl" -a -r "$dir/lib/libcrypto.so" -a -r "$dir/lib/libssl.so" && {
	    openssl="${dir}/bin/openssl";
	    SSL_BASE="$dir";
	    test "$dir" != '/usr' && {
	    	SSL_INCLUDE="-I${dir}/include";
	        SSL_LIB="-L${dir}/lib";
	    }
	    SSL_LIB="$SSL_LIB -lcrypto -lssl"
	    break;
        }
    done

#search for a random number generator . . .

    RNDF='';
    if [ ! -r /dev/random -o ! -r /dev/urandom ] ; then
      for file in /var/run/egd-pool /dev/egd-pool /etc/egd-pool /etc/entropy "$HOME/.rnd" .rnd ; do
        test -r $file && {
	  RNDF="$file";
	  break;
        }
      done
      if [ -z "$RNDF" ] ; then
        echo "";
        echo "Your OS does not provide you a random number generator.";
        echo "see http://www.lothar.com/tech/crypto on how to get EGD"
        echo "up and running, and restart config."
	echo "You could also generate some random data to another machine";
	echo "and put into a file named \`.rnd' placed in your HOME or in";
	echo "the current directory. This will seed the random"
	echo "number generator."
        test -n "$OS_SOLARIS" && { 
          echo "For solaris, you might also install the SUNski package";
          echo "from Sun patch 105710-01 (Sparc)."
        }
        echo "Visit http://www.openssl.org/support/faq.html#USER1 for details."
        echo "";
	openssl="";
      else
        echo "Using $RNDF as a random source . .";
	RNDF="-rand $RNDF";
      fi
    fi

    if [ ! -x "$openssl" -o -z "$openssl" ]; then
  	echo "";
  	echo "Cannot find the OpenSSL installation or one of its components !";
  	echo "SSL support for this build disabled.";
        echo " ";
  	USE_SSL="";
    fi
