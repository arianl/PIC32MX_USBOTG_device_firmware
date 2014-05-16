/* 
 * File:   cdc_stream.h
 * Author: Family
 *
 * Created on Antradienis, 2014, Vasario 18, 19.58
 */

#ifndef CDC_STREAM_H
#define	CDC_STREAM_H

#ifdef	__cplusplus
extern "C" {
#endif

int readCDC(void* data, int sz);
int writeCDC(const void* data, int sz);


#ifdef	__cplusplus
}
#endif

#endif	/* CDC_STREAM_H */

