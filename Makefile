# New ports collection makefile for:  Destroy
# Date created:         2003 March 07
# Date Modified:	2011 March 16 
# Whom:                 modsix@gmail.com
# $FreeBSD$

PORTNAME?=	destroy
CATEGORIES=     security
MASTER_SITES=   http://www.kinneysoft.com/destroy/ \
                http://www.impurity.org/mirrors/destroy/
MAINTAINER=     modsix@gmail.com
COMMENT=	Destory file on the harddrive in a secure manner.
PORTVERSION=	20110316
MAN1=		destroy.1
MANCOMPRESSED=	yes
USE_IMAKE=	yes
INSTALL=	/usr/bin/install -c -o root -g wheel
RM?=		${RM}
CC?=		${CC}
STRIP?=		${STRIP}
CFLAGS+=	-Wall -Werror
CFLAGS?=	${CFLAGS}
LDFLAGS?=
PLIST_FILES=	bin/destroy		

all:
	${CC} ${CFLAGS} ${LDFLAGS} -o ${PORTNAME} destroy.c

clean:
	${RM} destroy
	${RM} work

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/destroy ${PREFIX}/bin
	${INSTALL_MAN} ${WRKSRC}/destroy.1 ${MAN1PREFIX}/man/man1
 
linux:
	${CC} ${LDFLAGS} -o destroy destroy.c

deinstall:
	${RM} ${LOCALBASE}/bin/${PORTNAME}

post-install:
	${STRIP} ${LOCALBASE}/bin/${PORTNAME}

#.include <bsd.port.mk>
