suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))
suppressMessages(library(ggpmisc))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)
df <- filter(df, iterationsUntilValid != '2147483647')

df$strategy_f <- factor(df$strategy, levels=c('Default', 'Balanced'))
df$configured_f <- factor(df$configured, levels=c('Before', 'After'))

f <- y ~ x

plot <-
    ggplot(df, aes(x=log10_squared_edges,
                   y=iterationsUntilValid,
                   col=graph_type)) +
    geom_point() +
    geom_smooth(method="lm", formula=f, fullrange=TRUE) +
    stat_poly_eq(aes(label = ..eq.label..), formula=f, parse=TRUE) +
    labs(y="Iterations",
         x=expression("log"^2 ~ "m"),
         col='Graph type') +
    facet_grid(strategy_f ~ configured_f, scales="free")

ggsave(output_file, plot)
