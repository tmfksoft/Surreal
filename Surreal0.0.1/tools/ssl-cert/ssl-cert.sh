#!/bin/sh

CERT_DAYS=730
REBUILD_CRT="1"
ECHO='echo';
test -z `echo -n` && ECHO='echo -n';
. ssl-search.sh


	if [ -r "ircd.crt" ]; then
		echo " ";
		echo "*** You already have an SSL certificate . . .";
		echo " ";

		FOO=""
		runonce=""
		while [ -z "$FOO" ] ; do
		    FOO="No"
		    echo ""
		    echo "Do you want to rebuild your certificate ?";
		    $ECHO "[$FOO] -> $c"
		    if [ -z "$AUTO_CONFIG" -o -n "$runonce" ] ; then
			read cc
			runonce=Yes
		    else
			cc=""
		    fi
		    if [ -z "$cc" ] ; then
			cc=$FOO
		    fi
		    case "$cc" in
			[Yy]*)
			    REBUILD_CRT="1"
			    ;;
			[Nn]*)
			    REBUILD_CRT=""
			    ;;
			*)
			    echo ""
			    echo "You need to enter either Yes or No here..."
			    echo ""
			    FOO=""
			    ;;
		    esac
		done
	fi
	
	if [ -n "$REBUILD_CRT" ]; then
		echo " ";
		echo "*** Building a new SSL certificate for your server.";

		FOO=""
		runonce=""
		while [ -z "$FOO" ] ; do
		    FOO="$CERT_DAYS"
		    echo " "
		    echo "How many days will your certificate last ?"
		    echo " "
		    $ECHO "[$FOO] -> $c"
		    if [ -z "$AUTO_CONFIG" -o -n "$runonce" -o -z "$SERVICES_NAME" ] ; then
			read cc
			runonce=Yes
		    else
			cc=""
		    fi
		    if [ -z "$cc" ] ; then
			cc=$FOO
		    fi
		    case "$cc" in
		        *)
		            CERT_DAYS="$cc"
		    esac
		done
	
		$openssl req -new -x509 -days $CERT_DAYS -nodes \
			-config ../etc/ircdssl.cnf -out "../etc/ircd.crt" \
			-keyout "../etc/ircd.key" $RNDF
		$openssl x509 -subject -dates -fingerprint -noout \
			-in "../etc/ircd.crt"

	fi

	echo " "
	echo "*** SSL certificate step done."
	echo " "
