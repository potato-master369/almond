// cpu.c
// ---------------------------------
// CPU Management
#include "cpuid_c.h"
#include "mmu/kmalloc.h"
#include "messaging/messaging.h"
#include "clock/clock.h"
#include "helpers/string.h"
#include "cpu.h"

almond_cpu_t cpus[1];

static bool_t cpuid_supported(void) {
  uint32_t a, b;
  __asm__ volatile("pushfl; popl %0;"
                   "movl %0, %1; xorl $0x200000, %1;"
                   "pushl %1; popfl;"
                   "pushfl; popl %1;"
                   "pushl %0; popfl"
                   : "=&r"(a), "=&r"(b)
                   :
                   : "cc", "memory");
  return ((a ^ b) & 0x200000u) != 0;
}

static uint32_t rdtsc_lo(void) {
  uint32_t lo, hi;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return lo;
}

static uint32_t measure_mhz(void) {
  uint32_t t0 = rdtsc_lo();
  clock_wait_ms(50);
  uint32_t t1 = rdtsc_lo();
  return ((t1 - t0) + 25000u) / 50000u; /* ticks / (50ms * 1000) = MHz, rounded */
}


void cpu_init(void) {
  almond_cpuid_t r;
  uint32_t brand[12];
  char msgbuf[256];
  uint32_t max_leaf, family = 0, features_edx = 0;
  message_send_message(" cpu: enumerating CPUs...\n");
  message_send_message("WARN:cpu: this is practically a stub, so MP n shit doesnt work yet!\n");
  if (!cpuid_supported()) {
    message_send_message("WARN:cpu: CPUID is not supported. Will assume this is an \"*-Unknown*-* Intel 386/486\"\n");
    cpus[0].cpu_id = 0;
    cpus[0].core_id = 0;
    strcpy(cpus[0].prettyname, "Intel 386/486");
    strcpy(cpus[0].mfg, "*-Unknown*-*");
    return;
  }

  almond_cpuid_req(0, 0, &r);
  max_leaf = r.eax;
  memcpy(cpus[0].mfg + 0, &r.ebx, 4);
  memcpy(cpus[0].mfg + 4, &r.edx, 4);
  memcpy(cpus[0].mfg + 8, &r.ecx, 4);
  cpus[0].mfg[12] = '\0';

  if (max_leaf >= 1) {
    almond_cpuid_req(1, 0, &r);
    family = (r.eax >> 8) & 0xF;
    if (family == 0xF)
      family += (r.eax >> 20) & 0xFF;
    features_edx = r.edx;
  }
  strcpy(msgbuf, " cpu: mfg ");
  append_string(msgbuf, cpus[0].mfg, sizeof(msgbuf));
  append_string(msgbuf, "\n", sizeof(msgbuf));
  message_send_message(msgbuf);

  almond_cpuid_req(0x80000000u, 0, &r);
  if (r.eax >= 0x80000004u) {
    for (uint32_t i = 0; i < 3; ++i) {
      almond_cpuid_req(0x80000002u + i, 0, &r);
      brand[i * 4 + 0] = r.eax;
      brand[i * 4 + 1] = r.ebx;
      brand[i * 4 + 2] = r.ecx;
      brand[i * 4 + 3] = r.edx;
    }
    strcpy(cpus[0].prettyname, (char *)brand);
    cpus[0].prettyname[47] = 0;

    strcpy(msgbuf, " cpu: prettyname ");
    append_string(msgbuf, cpus[0].prettyname, sizeof(msgbuf));
    append_string(msgbuf, "\n", sizeof(msgbuf));
    message_send_message(msgbuf);
    return;
  }

  char cpubuf[49] = "";
  if (family >= 3 && family <= 6) {
    append_uint(cpubuf, family * 100 + 86, sizeof(cpubuf));
  } else {
    append_uint(cpubuf, family, sizeof(cpubuf));
  }
  append_string(cpubuf, " ", sizeof(cpubuf));
  append_uint(cpubuf, measure_mhz(), sizeof(cpubuf));
  append_string(cpubuf, " Mhz", sizeof(cpubuf));
  strcpy(cpus[0].prettyname, cpubuf);
  strcpy(msgbuf, " cpu: prettyname ");
  append_string(msgbuf, cpus[0].prettyname, sizeof(msgbuf));
  append_string(msgbuf, "\n", sizeof(msgbuf));
  message_send_message(msgbuf);
}
