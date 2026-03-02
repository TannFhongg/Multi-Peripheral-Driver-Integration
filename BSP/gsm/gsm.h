/* gsm.h - GSM module header */

#ifndef GSM_H
#define GSM_H

void GSM_Init(void);
void GSM_SendSMS(const char *number, const char *message);
void GSM_MakeCall(const char *number);

#endif /* GSM_H */
