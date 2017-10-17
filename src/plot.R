#!/usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE)

if (length(args) != 1)
    stop("missing input file <tfd500.csv>")

tfd500file <- args[1]
data <- read.csv(file = tfd500file, sep = ";", header = FALSE,
                 col.names = c("id", "date", "temperature", "humidity"))

data$date <- as.POSIXct(data$date, format = "%d.%m.%Y %H:%M:%S")
date.range <- c(as.POSIXlt(min(data$date)), as.POSIXlt(max(data$date)))

ylim.t <- c(min(data$temperature),
            max(data$temperature))
ylim.h = c(min(data$humidity),
           max(data$humidity))

png("plot.png", width = 10, height = 6,
    units = "in", res = 600)
par(mar = c(5, 5, 4, 6) + 0.1)

t.stats <- round(c(ylim.t, mean(data$temperature)), digits = 2)
h.stats <- round(c(ylim.h, mean(data$humidity)), digits = 2)

plot(data$date, data$temperature, pch = 16, axes = FALSE,
     ylim = ylim.t, lwd = 2.5,
     xlab = "", ylab = "",
     type = "l", col = "blue",
     sub = paste(date.range[1], "to", date.range[2]))

t.text <- paste("Temperature: min, max, mean", toString(t.stats), "[°C]")
h.text <- paste("\n\nHumidity: min, max, mean", toString(h.stats), "[%]")

title(t.text, col.main = "blue")
title(h.text, col.main = "red")

t.idx <- c(which.min(data$temperature), which.max(data$temperature))
points(data$date[t.idx[1]], data$temperature[t.idx[1]], pch = 10, cex = 2,
       col = "blue")
points(data$date[t.idx[2]], data$temperature[t.idx[2]], pch = 10, cex = 2,
       col = "blue")

axis(2, ylim = ylim.t, col = "blue", col.axis = "blue", las = 1, cex.axis = 1.25)
mtext("Temperature C°", side = 2, col = "blue", line = 2.5, cex = 1.25, padj = -1.5)
abline(h = t.stats[3], col = "blue", lty = 2)
box()

par(new=TRUE)
plot(data$date, data$humidity, pch = 15, axes = FALSE,
     xlab = "", ylab = "",
     ylim = ylim.h, lwd = 2.5,
     type = "l", col = "red")

h.idx <- c(which.min(data$humidity), which.max(data$humidity))
points(data$date[h.idx[1]], data$humidity[h.idx[1]], pch = 10, cex = 2,
       col = "red")
points(data$date[h.idx[2]], data$humidity[h.idx[2]], pch = 10, cex = 2,
       col = "red")

mtext("Humidity %", side = 4, col = "red", line = 4, cex  = 1.25)
axis(4, ylim = ylim.h, col = "red", col.axis = "red", las = 1, cex.axis = 1.25)

axis.POSIXct(1, at = seq(date.range[1], date.range[2], by="hour"), format="%H:%M", cex.axis = 1.25)
mtext("Time (hours)", side = 1, col = "black", line = 2.5)
abline(h = h.stats[3], col = "red", lty = 2)
grid(nx = 10, ny = 10)

dev.off()
