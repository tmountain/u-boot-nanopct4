// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015, Google, Inc
 * Copyright (C) 2014, Bin Meng <bmeng.cn@gmail.com>
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <sdhci.h>
#include <acpi/acpigen.h>
#include <acpi/acpi_device.h>
#include <acpi/acpi_dp.h>
#include <asm-generic/gpio.h>
#include <dm/acpi.h>

struct pci_mmc_plat {
	struct mmc_config cfg;
	struct mmc mmc;
};

struct pci_mmc_priv {
	struct sdhci_host host;
	void *base;
	struct gpio_desc cd_gpio;
};

static int pci_mmc_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct pci_mmc_plat *plat = dev_get_platdata(dev);
	struct pci_mmc_priv *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	int ret;

	host->ioaddr = (void *)dm_pci_map_bar(dev, PCI_BASE_ADDRESS_0,
					      PCI_REGION_MEM);
	host->name = dev->name;
	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	ret = sdhci_setup_cfg(&plat->cfg, host, 0, 0);
	if (ret)
		return ret;
	host->mmc->priv = &priv->host;
	upriv->mmc = host->mmc;

	return sdhci_probe(dev);
}

static int pci_mmc_ofdata_to_platdata(struct udevice *dev)
{
	struct pci_mmc_priv *priv = dev_get_priv(dev);

	gpio_request_by_name(dev, "cd-gpios", 0, &priv->cd_gpio, GPIOD_IS_IN);

	return 0;
}

static int pci_mmc_bind(struct udevice *dev)
{
	struct pci_mmc_plat *plat = dev_get_platdata(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static int pci_mmc_acpi_fill_ssdt(const struct udevice *dev,
				  struct acpi_ctx *ctx)
{
	struct pci_mmc_priv *priv = dev_get_priv(dev);
	char path[ACPI_PATH_MAX];
	struct acpi_gpio gpio;
	struct acpi_dp *dp;
	int ret;

	if (!dev_of_valid(dev))
		return 0;

	ret = gpio_get_acpi(&priv->cd_gpio, &gpio);
	if (ret)
		return log_msg_ret("gpio", ret);
	gpio.type = ACPI_GPIO_TYPE_INTERRUPT;
	gpio.pull = ACPI_GPIO_PULL_NONE;
	gpio.irq.mode = ACPI_IRQ_EDGE_TRIGGERED;
	gpio.irq.polarity = ACPI_IRQ_ACTIVE_BOTH;
	gpio.irq.shared = ACPI_IRQ_SHARED;
	gpio.irq.wake = ACPI_IRQ_WAKE;
	gpio.interrupt_debounce_timeout = 10000; /* 100ms */

	/* Use device path as the Scope for the SSDT */
	ret = acpi_device_path(dev, path, sizeof(path));
	if (ret)
		return log_msg_ret("path", ret);
	acpigen_write_scope(ctx, path);
	acpigen_write_name(ctx, "_CRS");

	/* Write GpioInt() as default (if set) or custom from devicetree */
	acpigen_write_resourcetemplate_header(ctx);
	acpi_device_write_gpio(ctx, &gpio);
	acpigen_write_resourcetemplate_footer(ctx);

	/* Bind the cd-gpio name to the GpioInt() resource */
	dp = acpi_dp_new_table("_DSD");
	if (!dp)
		return -ENOMEM;
	acpi_dp_add_gpio(dp, "cd-gpio", path, 0, 0, 1);
	ret = acpi_dp_write(ctx, dp);
	if (ret)
		return log_msg_ret("cd", ret);

	acpigen_pop_len(ctx);

	return 0;
}

struct acpi_ops pci_mmc_acpi_ops = {
	.fill_ssdt	= pci_mmc_acpi_fill_ssdt,
};

static const struct udevice_id pci_mmc_match[] = {
	{ .compatible = "intel,apl-sd" },
	{ }
};

U_BOOT_DRIVER(pci_mmc) = {
	.name	= "pci_mmc",
	.id	= UCLASS_MMC,
	.of_match = pci_mmc_match,
	.bind	= pci_mmc_bind,
	.ofdata_to_platdata	= pci_mmc_ofdata_to_platdata,
	.probe	= pci_mmc_probe,
	.ops	= &sdhci_ops,
	.priv_auto_alloc_size = sizeof(struct pci_mmc_priv),
	.platdata_auto_alloc_size = sizeof(struct pci_mmc_plat),
	ACPI_OPS_PTR(&pci_mmc_acpi_ops)
};

static struct pci_device_id mmc_supported[] = {
	{ PCI_DEVICE_CLASS(PCI_CLASS_SYSTEM_SDHCI << 8, 0xffff00) },
	{},
};

U_BOOT_PCI_DEVICE(pci_mmc, mmc_supported);
