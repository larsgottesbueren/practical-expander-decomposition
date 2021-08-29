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
    ggplot(df, aes(x=iteration, y=potential, color=strategy_type)) +
    scale_y_log10(
        breaks = scales::trans_breaks("log10", function(x) 10^x),
        labels = scales::trans_format("log10", scales::math_format(10^.x))
    ) +
    labs(y="Potential",
         x="Iteration",
         col="Strategy type") +
    stat_summary(fun=median, geom="line") +
    facet_wrap(graph ~ .)

ggsave(output_file, plot)
