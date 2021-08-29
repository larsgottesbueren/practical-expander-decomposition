suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

df$strategy_type <- gsub("Default", "Original", df$strategy_type)

plot <-
    ggplot(df, aes(x=phi, y=certificate, col=ifelse(certificate >= phi, 'Correct', 'Incorrect'))) +
    geom_point() +
    geom_abline(intercept=0, slope=1) +
    labs(x=expression(phi),
         y=expression("Certificate" ~ phi[c]),
         col="Satisfies requirement") +
    facet_grid(graph ~ strategy_type, label="label_parsed") +
    scale_x_continuous(breaks=c(0.0001, 0.0025, 0.005, 0.0075, 0.01), limits = c(0.0001, NA)) +
    scale_y_continuous(limits = c(0.0, NA)) +
    theme(panel.spacing = unit(2, "lines")) +
    scale_color_manual(values = c('Correct' = "blue", 'Incorrect' = "red"))

ggsave(output_file, plot)
